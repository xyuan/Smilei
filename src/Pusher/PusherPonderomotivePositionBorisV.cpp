#include "PusherPonderomotivePositionBorisV.h"

#include <iostream>
#include <cmath>

#include "Species.h"

#include "Particles.h"

using namespace std;
// Pushes only position of particles interacting with envelope, not their momentum
PusherPonderomotivePositionBorisV::PusherPonderomotivePositionBorisV(Params& params, Species *species)
    : Pusher(params, species)
{
}

PusherPonderomotivePositionBorisV::~PusherPonderomotivePositionBorisV()
{
}

/**************************************************************************
    Lorentz Force + Ponderomotive force -- leap-frog (Boris-style) scheme, position advance
**************************************************************************/

void PusherPonderomotivePositionBorisV::operator() (Particles &particles, SmileiMPI* smpi, int istart, int iend, int ithread, int ipart_ref)
{
/////////// not vectorized
    std::vector<double> *Phipart        = &(smpi->dynamics_PHIpart[ithread]);
    std::vector<double> *GradPhipart    = &(smpi->dynamics_GradPHIpart[ithread]);
    std::vector<double> *Phioldpart     = &(smpi->dynamics_PHIoldpart[ithread]);
    std::vector<double> *GradPhioldpart = &(smpi->dynamics_GradPHIoldpart[ithread]);
    
    double charge_sq_over_mass_dts4,charge_over_mass_sq;
    double gamma0,gamma0_sq,gamma_ponderomotive;
    double pxsm, pysm, pzsm;
    
    //int* cell_keys;

    double* momentum[3];
    for ( int i = 0 ; i<3 ; i++ )
        momentum[i] =  &( particles.momentum(i,0) );
    double* position[3];
    for ( int i = 0 ; i<nDim_ ; i++ )
        position[i] =  &( particles.position(i,0) );
#ifdef  __DEBUG
    double* position_old[3];
    for ( int i = 0 ; i<nDim_ ; i++ )
        position_old[i] =  &( particles.position_old(i,0) );
#endif
    
    short* charge = &( particles.charge(0) );
    
    int nparts = GradPhipart->size()/3;
   
    double* Phi         = &( (*Phipart)[0*nparts] );
    double* Phiold      = &( (*Phioldpart)[0*nparts] );
    double* GradPhix    = &( (*GradPhipart)[0*nparts] );
    double* GradPhiy    = &( (*GradPhipart)[1*nparts] );
    double* GradPhiz    = &( (*GradPhipart)[2*nparts] );
    double* GradPhioldx = &( (*GradPhioldpart)[0*nparts] );
    double* GradPhioldy = &( (*GradPhioldpart)[1*nparts] );
    double* GradPhioldz = &( (*GradPhioldpart)[2*nparts] );
    
    //particles.cell_keys.resize(nparts);
    //cell_keys = &( particles.cell_keys[0]);

    #pragma omp simd
    for (int ipart=istart ; ipart<iend; ipart++ ) { // begin loop on particles
    
        // ! ponderomotive force is proportional to charge squared and the field is divided by 4 instead of 2
        charge_sq_over_mass_dts4 = (double)(charge[ipart])*(double)(charge[ipart])*one_over_mass_*dts4;         
        // (charge over mass)^2
        charge_over_mass_sq      = (double)(charge[ipart])*one_over_mass_*(charge[ipart])*one_over_mass_;

        // compute initial ponderomotive gamma 
        gamma0_sq = 1. + momentum[0][ipart]*momentum[0][ipart] + momentum[1][ipart]*momentum[1][ipart] + momentum[2][ipart]*momentum[2][ipart] + (*(Phi+ipart-ipart_ref)+*(Phiold+ipart-ipart_ref))*charge_over_mass_sq*0.5 ;
        gamma0    = sqrt(gamma0_sq) ;      
  
        // ponderomotive force for ponderomotive gamma advance (Grad Phi is interpolated in time, hence the division by 2)
        pxsm = charge_sq_over_mass_dts4 * ( *(GradPhix+ipart-ipart_ref) + *(GradPhioldx+ipart-ipart_ref) ) * 0.5 / gamma0_sq ;
        pysm = charge_sq_over_mass_dts4 * ( *(GradPhiy+ipart-ipart_ref) + *(GradPhioldy+ipart-ipart_ref) ) * 0.5 / gamma0_sq ;
        pzsm = charge_sq_over_mass_dts4 * ( *(GradPhiz+ipart-ipart_ref) + *(GradPhioldz+ipart-ipart_ref) ) * 0.5 / gamma0_sq ;

        // update of gamma ponderomotive 
        gamma_ponderomotive = gamma0 + (pxsm*momentum[0][ipart]+pysm*momentum[1][ipart]+pzsm*momentum[2][ipart]) ;

        // Move the particle
#ifdef  __DEBUG
        for ( int i = 0 ; i<nDim_ ; i++ ) 
          position_old[i][ipart] = position[i][ipart];
#endif
        for ( int i = 0 ; i<nDim_ ; i++ ) 
            position[i][ipart]     += dt*momentum[i][ipart]/gamma_ponderomotive;

    } // end loop on particles

    //#pragma omp simd
    //for (int ipart=0 ; ipart<nparts; ipart++ ) {
    //
    //    for ( int i = 0 ; i<nDim_ ; i++ ){ 
    //        cell_keys[ipart] *= nspace[i];
    //        cell_keys[ipart] += round( (position[i][ipart]-min_loc_vec[i]) * dx_inv_[i] );
    //    }
    //    
    //} // end loop on particles


}
