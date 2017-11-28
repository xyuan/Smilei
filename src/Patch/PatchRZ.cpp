
#include "PatchRZ.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "Hilbert_functions.h"
#include "PatchesFactory.h"
#include "Species.h"
#include "Particles.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// PatchRZ constructor 
// ---------------------------------------------------------------------------------------------------------------------
PatchRZ::PatchRZ(Params& params, SmileiMPI* smpi, unsigned int ipatch, unsigned int n_moved)
  : Patch( params, smpi, ipatch, n_moved)
{
    initStep2(params);
    initStep3(params, smpi, n_moved);
    finishCreation(params, smpi);
} // End PatchRZ::PatchRZ


// ---------------------------------------------------------------------------------------------------------------------
// PatchRZ cloning constructor 
// ---------------------------------------------------------------------------------------------------------------------
PatchRZ::PatchRZ(PatchRZ* patch, Params& params, SmileiMPI* smpi, unsigned int ipatch, unsigned int n_moved, bool with_particles = true)
  : Patch( patch, params, smpi, ipatch, n_moved, with_particles)
{
    initStep2(params);
    initStep3(params, smpi, n_moved);
    finishCloning(patch, params, smpi, with_particles);
} // End PatchRZ::PatchRZ


// ---------------------------------------------------------------------------------------------------------------------
// PatchRZ second initializer :
//   - Pcoordinates, neighbor_ resized in Patch constructor 
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::initStep2(Params& params)
{
    int xcall, ycall;
    
    // define patch coordinates
    Pcoordinates.resize(2);
    generalhilbertindexinv(params.mi[0], params.mi[1], &Pcoordinates[0], &Pcoordinates[1], hindex);
#ifdef _DEBUG
    cout << "\tPatch coords : ";
    for (int iDim=0; iDim<2;iDim++)
        cout << "\t" << Pcoordinates[iDim] << " ";
    cout << endl;
#endif
    

    // 1st direction
    xcall = Pcoordinates[0]-1;
    ycall = Pcoordinates[1];
    if (params.bc_em_type_x[0]=="periodic" && xcall < 0) xcall += (1<<params.mi[0]);
    neighbor_[0][0] = generalhilbertindex( params.mi[0], params.mi[1], xcall, ycall);
    xcall = Pcoordinates[0]+1;
    if (params.bc_em_type_x[0]=="periodic" && xcall >= (1<<params.mi[0])) xcall -= (1<<params.mi[0]);
    neighbor_[0][1] = generalhilbertindex( params.mi[0], params.mi[1], xcall, ycall);

    // 2st direction
    xcall = Pcoordinates[0];
    ycall = Pcoordinates[1]-1;
    if (params.bc_em_type_y[0]=="periodic" && ycall < 0) ycall += (1<<params.mi[1]);
    neighbor_[1][0] =  generalhilbertindex( params.mi[0], params.mi[1], xcall, ycall);
    ycall = Pcoordinates[1]+1;
    if (params.bc_em_type_y[0]=="periodic" && ycall >= (1<<params.mi[1])) ycall -= (1<<params.mi[1]);
    neighbor_[1][1] =  generalhilbertindex( params.mi[0], params.mi[1], xcall, ycall);

    for (int ix_isPrim=0 ; ix_isPrim<2 ; ix_isPrim++) {
        for (int iy_isPrim=0 ; iy_isPrim<2 ; iy_isPrim++) {
            ntype_[0][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            ntype_[1][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            ntypeSum_[0][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            ntypeSum_[1][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            
        }
    }

}


PatchRZ::~PatchRZ()
{
    cleanType();
}


void PatchRZ::reallyinitSumField( Field* field, int iDim )
{
}

// ---------------------------------------------------------------------------------------------------------------------
// Initialize current patch sum Fields communications through MPI in direction iDim
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::initSumField( Field* field, int iDim )
{
    if (field->MPIbuff.ibuf[0][0].size()==0) {
        field->MPIbuff.allocate(2, field, oversize);

        int tagp(0);
        if (field->name == "Jx") tagp = 1;
        if (field->name == "Jy") tagp = 2;
        if (field->name == "Jz") tagp = 3;
        if (field->name == "Rho") tagp = 4;

        field->MPIbuff.defineTags( this, tagp );
    }

    int patch_ndims_(2);
    int patch_nbNeighbors_(2);
    
    std::vector<unsigned int> n_elem = field->dims_;
    std::vector<unsigned int> isDual = field->isDual_;
    cField2D* f3D =  static_cast<cField2D*>(field);
   
    // Use a buffer per direction to exchange data before summing
    //cField2D buf[patch_ndims_][patch_nbNeighbors_];
    // Size buffer is 2 oversize (1 inside & 1 outside of the current subdomain)
    std::vector<unsigned int> oversize2 = oversize;
    oversize2[0] *= 2;
    oversize2[0] += 1 + f3D->isDual_[0];
    if (field->dims_.size()>1) {
        oversize2[1] *= 2;
        oversize2[1] += 1 + f3D->isDual_[1];
    }

    vector<int> idx( patch_ndims_,1 );
    idx[iDim] = 0;    

    int istart, ix, iy;
    /********************************************************************************/
    // Send/Recv in a buffer data to sum
    /********************************************************************************/
    memset(&(idx[0]), 0, sizeof(idx[0])*idx.size());
    idx[iDim] = 1;    
        
    MPI_Datatype ntype = ntypeSum_[iDim][isDual[0]][isDual[1]];
        
    for (int iNeighbor=0 ; iNeighbor<patch_nbNeighbors_ ; iNeighbor++) {
            
        if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {
            istart = iNeighbor * ( n_elem[iDim]- oversize2[iDim] ) + (1-iNeighbor) * ( 0 );
            ix = idx[0]*istart;
            iy = idx[1]*istart;
            int tag = f3D->MPIbuff.send_tags_[iDim][iNeighbor];
            MPI_Isend( &((*f3D)(ix,iy)), 1, ntype, MPI_neighbor_[iDim][iNeighbor], tag, 
                       MPI_COMM_WORLD, &(f3D->MPIbuff.srequest[iDim][iNeighbor]) );
        } // END of Send
            
        if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {
            int tmp_elem = f3D->MPIbuff.ibuf[iDim][(iNeighbor+1)%2].size();
            int tag = f3D->MPIbuff.recv_tags_[iDim][iNeighbor];
            MPI_Irecv( &( f3D->MPIbuff.ibuf[iDim][(iNeighbor+1)%2][0] ), 2*tmp_elem, MPI_DOUBLE, MPI_neighbor_[iDim][(iNeighbor+1)%2], tag, 
                       MPI_COMM_WORLD, &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]) );
        } // END of Recv
            
    } // END for iNeighbor

} // END initSumField


// ---------------------------------------------------------------------------------------------------------------------
// Finalize current patch sum Fields communications through MPI for direction iDim
// Proceed to the local reduction
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::finalizeSumField( Field* field, int iDim )
{
    int patch_ndims_(2);
//    int patch_nbNeighbors_(2);
    std::vector<unsigned int> n_elem = field->dims_;
    std::vector<unsigned int> isDual = field->isDual_;
    cField2D* f3D =  static_cast<cField2D*>(field);
   
    // Use a buffer per direction to exchange data before summing
    //cField2D buf[patch_ndims_][patch_nbNeighbors_];
    // Size buffer is 2 oversize (1 inside & 1 outside of the current subdomain)
    std::vector<unsigned int> oversize2 = oversize;
    oversize2[0] *= 2;
    oversize2[0] += 1 + f3D->isDual_[0];
    oversize2[1] *= 2;
    oversize2[1] += 1 + f3D->isDual_[1];
    
    /********************************************************************************/
    // Send/Recv in a buffer data to sum
    /********************************************************************************/

    MPI_Status sstat    [patch_ndims_][2];
    MPI_Status rstat    [patch_ndims_][2];
        
    for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
        if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {
            MPI_Wait( &(f3D->MPIbuff.srequest[iDim][iNeighbor]), &(sstat[iDim][iNeighbor]) );
        }
        if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {
            MPI_Wait( &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]), &(rstat[iDim][(iNeighbor+1)%2]) );
        }
    }
    
    int istart;
    /********************************************************************************/
    // Sum data on each process, same operation on both side
    /********************************************************************************/
    vector<int> idx( 2, 1 );
    idx[iDim] = 0;
    std::vector<unsigned int> tmp(2,0);
    tmp[0] =    idx[0]  * n_elem[0] + (1-idx[0]) * oversize2[0];
    tmp[1] =    idx[1]  * n_elem[1] + (1-idx[1]) * oversize2[1];

    memset(&(idx[0]), 0, sizeof(idx[0])*idx.size());
    idx[iDim] = 1;    
   
    for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

        istart = ( (iNeighbor+1)%2 ) * ( n_elem[iDim]- oversize2[iDim] ) + (1-(iNeighbor+1)%2) * ( 0 );
        int ix0 = idx[0]*istart;
        int iy0 = idx[1]*istart;
        if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {
            for (unsigned int ix=0 ; ix< tmp[0] ; ix++) {
                for (unsigned int iy=0 ; iy< tmp[1] ; iy++) {
                    (*f3D)(ix0+ix,iy0+iy) += f3D->MPIbuff.ibuf[iDim][(iNeighbor+1)%2][ix*tmp[1] + iy];
                }
            }
        } // END if
            
    } // END for iNeighbor
      
} // END finalizeSumField


void PatchRZ::reallyfinalizeSumField( Field* field, int iDim )
{
} // END finalizeSumField


// ---------------------------------------------------------------------------------------------------------------------
// Initialize current patch exhange Fields communications through MPI (includes loop / nDim_fields_)
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::initExchange( Field* field )
{
    cout << "On ne passe jamais ici !!!!" << endl;

    if (field->MPIbuff.srequest.size()==0)
        field->MPIbuff.allocate(2);

    int patch_ndims_(2);
    int patch_nbNeighbors_(2);

    std::vector<unsigned int> n_elem   = field->dims_;
    std::vector<unsigned int> isDual = field->isDual_;
    cField2D* f3D =  static_cast<cField2D*>(field);

    int istart, ix, iy;

    // Loop over dimField
    for (int iDim=0 ; iDim<patch_ndims_ ; iDim++) {

	vector<int> idx( patch_ndims_,0 );
	idx[iDim] = 1;

        MPI_Datatype ntype = ntype_[iDim][isDual[0]][isDual[1]];
        for (int iNeighbor=0 ; iNeighbor<patch_nbNeighbors_ ; iNeighbor++) {

            if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {

                istart = iNeighbor * ( n_elem[iDim]- (2*oversize[iDim]+1+isDual[iDim]) ) + (1-iNeighbor) * ( oversize[iDim] + 1 + isDual[iDim] );
                ix = idx[0]*istart;
                iy = idx[1]*istart;
                int tag = buildtag( hindex, iDim, iNeighbor );
                MPI_Isend( &((*f3D)(ix,iy)), 1, ntype, MPI_neighbor_[iDim][iNeighbor], tag, 
                           MPI_COMM_WORLD, &(f3D->MPIbuff.srequest[iDim][iNeighbor]) );

            } // END of Send

            if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {

                istart = ( (iNeighbor+1)%2 ) * ( n_elem[iDim] - 1 - (oversize[iDim]-1) ) + (1-(iNeighbor+1)%2) * ( 0 )  ;
                ix = idx[0]*istart;
                iy = idx[1]*istart;
                int tag = buildtag( neighbor_[iDim][(iNeighbor+1)%2], iDim, iNeighbor );
                MPI_Irecv( &((*f3D)(ix,iy)), 1, ntype, MPI_neighbor_[iDim][(iNeighbor+1)%2], tag, 
                           MPI_COMM_WORLD, &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]));

            } // END of Recv

        } // END for iNeighbor

    } // END for iDim
} // END initExchange( Field* field )


// ---------------------------------------------------------------------------------------------------------------------
// Initialize current patch exhange Fields communications through MPI  (includes loop / nDim_fields_)
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::finalizeExchange( Field* field )
{
    cField2D* f3D =  static_cast<cField2D*>(field);

    int patch_ndims_(2);
    MPI_Status sstat    [patch_ndims_][2];
    MPI_Status rstat    [patch_ndims_][2];

    // Loop over dimField
    for (int iDim=0 ; iDim<patch_ndims_ ; iDim++) {

        for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
            if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {
                MPI_Wait( &(f3D->MPIbuff.srequest[iDim][iNeighbor]), &(sstat[iDim][iNeighbor]) );
            }
             if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {
               MPI_Wait( &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]), &(rstat[iDim][(iNeighbor+1)%2]) );
            }
        }

    } // END for iDim

} // END finalizeExchange( Field* field )


// ---------------------------------------------------------------------------------------------------------------------
// Initialize current patch exhange Fields communications through MPI for direction iDim
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::initExchange( Field* field, int iDim )
{
    if (field->MPIbuff.srequest.size()==0){
        field->MPIbuff.allocate(2);

        int tagp(0);
        if (field->name == "Bx") tagp = 6;
        if (field->name == "By") tagp = 7;
        if (field->name == "Bz") tagp = 8;

        field->MPIbuff.defineTags( this, tagp );
    }

    int patch_ndims_(2);
    int patch_nbNeighbors_(2);

    vector<int> idx( patch_ndims_,0 );
    idx[iDim] = 1;

    std::vector<unsigned int> n_elem   = field->dims_;
    std::vector<unsigned int> isDual = field->isDual_;
    cField2D* f3D =  static_cast<cField2D*>(field);

    int istart, ix, iy;

    MPI_Datatype ntype = ntype_[iDim][isDual[0]][isDual[1]];
    for (int iNeighbor=0 ; iNeighbor<patch_nbNeighbors_ ; iNeighbor++) {

        if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {

            istart = iNeighbor * ( n_elem[iDim]- (2*oversize[iDim]+1+isDual[iDim]) ) + (1-iNeighbor) * ( oversize[iDim] + 1 + isDual[iDim] );
            ix = idx[0]*istart;
            iy = idx[1]*istart;
            int tag = f3D->MPIbuff.send_tags_[iDim][iNeighbor];
            MPI_Isend( &((*f3D)(ix,iy)), 1, ntype, MPI_neighbor_[iDim][iNeighbor], tag, 
                       MPI_COMM_WORLD, &(f3D->MPIbuff.srequest[iDim][iNeighbor]) );

        } // END of Send

        if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {

            istart = ( (iNeighbor+1)%2 ) * ( n_elem[iDim] - 1- (oversize[iDim]-1) ) + (1-(iNeighbor+1)%2) * ( 0 )  ;
            ix = idx[0]*istart;
            iy = idx[1]*istart;
            int tag = f3D->MPIbuff.recv_tags_[iDim][iNeighbor];
            MPI_Irecv( &((*f3D)(ix,iy)), 1, ntype, MPI_neighbor_[iDim][(iNeighbor+1)%2], tag, 
                       MPI_COMM_WORLD, &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]));

        } // END of Recv

    } // END for iNeighbor


} // END initExchange( Field* field, int iDim )


// ---------------------------------------------------------------------------------------------------------------------
// Initialize current patch exhange Fields communications through MPI for direction iDim
// Intra-MPI process communications managed by memcpy in SyncVectorPatch::sum()
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::finalizeExchange( Field* field, int iDim )
{
    int patch_ndims_(2);

    cField2D* f3D =  static_cast<cField2D*>(field);

    MPI_Status sstat    [patch_ndims_][2];
    MPI_Status rstat    [patch_ndims_][2];

    for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
        if ( is_a_MPI_neighbor( iDim, iNeighbor ) ) {
            MPI_Wait( &(f3D->MPIbuff.srequest[iDim][iNeighbor]), &(sstat[iDim][iNeighbor]) );
        }
        if ( is_a_MPI_neighbor( iDim, (iNeighbor+1)%2 ) ) {
            MPI_Wait( &(f3D->MPIbuff.rrequest[iDim][(iNeighbor+1)%2]), &(rstat[iDim][(iNeighbor+1)%2]) );
        }
    }

} // END finalizeExchange( Field* field, int iDim )


// ---------------------------------------------------------------------------------------------------------------------
// Create MPI_Datatypes used in initSumField and initExchange
// ---------------------------------------------------------------------------------------------------------------------
void PatchRZ::createType( Params& params )
{
    if (ntype_[0][0][0] != MPI_DATATYPE_NULL)
        return;

    int nx0 = params.n_space[0] + 1 + 2*params.oversize[0];
    int ny0 = params.n_space[1] + 1 + 2*params.oversize[1];
    
    // MPI_Datatype ntype_[nDim][primDual][primDual]
    int nx, ny;
    int nx_sum, ny_sum;
    
    for (int ix_isPrim=0 ; ix_isPrim<2 ; ix_isPrim++) {
        nx = nx0 + ix_isPrim;
        for (int iy_isPrim=0 ; iy_isPrim<2 ; iy_isPrim++) {
            ny = ny0 + iy_isPrim;
            
            // Standard Type
            ntype_[0][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            MPI_Type_contiguous(2*params.oversize[0]*ny, 
                                MPI_DOUBLE, &(ntype_[0][ix_isPrim][iy_isPrim]));
            MPI_Type_commit( &(ntype_[0][ix_isPrim][iy_isPrim]) );

            ntype_[1][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            MPI_Type_vector(2*nx, params.oversize[1], ny, 
                            MPI_DOUBLE, &(ntype_[1][ix_isPrim][iy_isPrim]));
            MPI_Type_commit( &(ntype_[1][ix_isPrim][iy_isPrim]) );


            nx_sum = 1 + 2*params.oversize[0] + ix_isPrim;
            ny_sum = 1 + 2*params.oversize[1] + iy_isPrim;

            ntypeSum_[0][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            MPI_Type_contiguous(2*nx_sum*ny, 
                                MPI_DOUBLE, &(ntypeSum_[0][ix_isPrim][iy_isPrim]));
            MPI_Type_commit( &(ntypeSum_[0][ix_isPrim][iy_isPrim]) );
            
            ntypeSum_[1][ix_isPrim][iy_isPrim] = MPI_DATATYPE_NULL;
            MPI_Type_vector(2*nx, ny_sum, ny, 
                            MPI_DOUBLE, &(ntypeSum_[1][ix_isPrim][iy_isPrim]));
            MPI_Type_commit( &(ntypeSum_[1][ix_isPrim][iy_isPrim]) );

        }
    }
    
} //END createType

void PatchRZ::cleanType()
{
    if (ntype_[0][0][0] == MPI_DATATYPE_NULL)
        return;

    for (int ix_isPrim=0 ; ix_isPrim<2 ; ix_isPrim++) {
        for (int iy_isPrim=0 ; iy_isPrim<2 ; iy_isPrim++) {
                MPI_Type_free( &(ntype_[0][ix_isPrim][iy_isPrim]) );
                MPI_Type_free( &(ntype_[1][ix_isPrim][iy_isPrim]) );
                MPI_Type_free( &(ntypeSum_[0][ix_isPrim][iy_isPrim]) );
                MPI_Type_free( &(ntypeSum_[1][ix_isPrim][iy_isPrim]) );    
        }
    }
}
