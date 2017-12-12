#include "Projector2D2OrderV.h"

#include <cmath>
#include <iostream>

#include "ElectroMagn.h"
#include "Field2D.h"
#include "Particles.h"
#include "Tools.h"
#include "Patch.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// Constructor for Projector2D2OrderV
// ---------------------------------------------------------------------------------------------------------------------
Projector2D2OrderV::Projector2D2OrderV (Params& params, Patch* patch) : Projector2D(params, patch)
{
    dx_inv_   = 1.0/params.cell_length[0];
    dx_ov_dt  = params.cell_length[0] / params.timestep;
    dy_inv_   = 1.0/params.cell_length[1];
    dy_ov_dt  = params.cell_length[1] / params.timestep;
    
    one_third = 1.0/3.0;

    i_domain_begin = patch->getCellStartingGlobalIndex(0);
    j_domain_begin = patch->getCellStartingGlobalIndex(1);

    nprimy = params.n_space[1] + 1;
    oversize[0] = params.oversize[0];
    oversize[1] = params.oversize[1];
    dq_inv[0] = dx_inv_;
    dq_inv[1] = dy_inv_;


    DEBUG("cell_length "<< params.cell_length[0]);

}


// ---------------------------------------------------------------------------------------------------------------------
// Destructor for Projector2D2OrderV
// ---------------------------------------------------------------------------------------------------------------------
Projector2D2OrderV::~Projector2D2OrderV()
{
}

// ---------------------------------------------------------------------------------------------------------------------
//!  Project current densities & charge : diagFields timstep (not vectorized)
// ---------------------------------------------------------------------------------------------------------------------
void Projector2D2OrderV::operator() (double* Jx, double* Jy, double* Jz, double* rho, Particles &particles, unsigned int ipart, double invgf, unsigned int bin, std::vector<unsigned int> &b_dim, int* iold, double* deltaold)
{
    
    // -------------------------------------
    // Variable declaration & initialization
    // -------------------------------------
    
    int iloc;
    // (x,y,z) components of the current density for the macro-particle
    double charge_weight = (double)(particles.charge(ipart))*particles.weight(ipart);
    double crx_p = charge_weight*dx_ov_dt;
    double cry_p = charge_weight*dy_ov_dt;
    double crz_p = charge_weight*particles.momentum(2, ipart)*invgf;
    
    
    // variable declaration
    double xpn, ypn;
    double delta, delta2;
    // arrays used for the Esirkepov projection method
    double  Sx0[5], Sx1[5], Sy0[5], Sy1[5], DSx[5], DSy[5], tmpJx[5];
    
    for (unsigned int i=0; i<5; i++) {
        Sx1[i] = 0.;
        Sy1[i] = 0.;
        // local array to accumulate Jx
        tmpJx[i] = 0.;
    }
    Sx0[0] = 0.;
    Sx0[4] = 0.;
    Sy0[0] = 0.;
    Sy0[4] = 0.;
    
    int nparts = particles.size();
    // --------------------------------------------------------
    // Locate particles & Calculate Esirkepov coef. S, DS and W
    // --------------------------------------------------------
    
    // locate the particle on the primal grid at former time-step & calculate coeff. S0
    delta = *deltaold;
    delta2 = delta*delta;
    Sx0[1] = 0.5 * (delta2-delta+0.25);
    Sx0[2] = 0.75-delta2;
    Sx0[3] = 0.5 * (delta2+delta+0.25);
    
    delta = *(deltaold+nparts);
    delta2 = delta*delta;
    Sy0[1] = 0.5 * (delta2-delta+0.25);
    Sy0[2] = 0.75-delta2;
    Sy0[3] = 0.5 * (delta2+delta+0.25);
    
    
    // locate the particle on the primal grid at current time-step & calculate coeff. S1
    xpn = particles.position(0, ipart) * dx_inv_;
    int ip = round(xpn);
    int ipo = iold[0];
    int ip_m_ipo = ip-ipo-i_domain_begin;
    delta  = xpn - (double)ip;
    delta2 = delta*delta;
    Sx1[ip_m_ipo+1] = 0.5 * (delta2-delta+0.25);
    Sx1[ip_m_ipo+2] = 0.75-delta2;
    Sx1[ip_m_ipo+3] = 0.5 * (delta2+delta+0.25);
    
    ypn = particles.position(1, ipart) * dy_inv_;
    int jp = round(ypn);
    int jpo = iold[1];
    int jp_m_jpo = jp-jpo-j_domain_begin;
    delta  = ypn - (double)jp;
    delta2 = delta*delta;
    Sy1[jp_m_jpo+1] = 0.5 * (delta2-delta+0.25);
    Sy1[jp_m_jpo+2] = 0.75-delta2;
    Sy1[jp_m_jpo+3] = 0.5 * (delta2+delta+0.25);
    
    for (unsigned int i=0; i < 5; i++) {
        DSx[i] = Sx1[i] - Sx0[i];
        DSy[i] = Sy1[i] - Sy0[i];
    }
    
    // calculate Esirkepov coeff. Wx, Wy, Wz when used
    double tmp, tmp2, tmp3, tmpY;
    //Do not compute useless weights.
    // ------------------------------------------------
    // Local current created by the particle
    // calculate using the charge conservation equation
    // ------------------------------------------------
    
    // ---------------------------
    // Calculate the total current
    // ---------------------------
    ipo -= bin+2; //This minus 2 come from the order 2 scheme, based on a 5 points stencil from -2 to +2.
    jpo -= 2;
    // case i =0
    {
        iloc = ipo*b_dim[1]+jpo;
        tmp2 = 0.5*Sx1[0];
        tmp3 =     Sx1[0];
        Jz[iloc]  += crz_p * one_third * ( Sy1[0]*tmp3 );
        rho[iloc] += charge_weight * Sx1[0]*Sy1[0];
        tmp = 0;
        tmpY = Sx0[0] + 0.5*DSx[0];
        for (unsigned int j=1 ; j<5 ; j++) {
            tmp -= cry_p * DSy[j-1] * tmpY;
            Jy[iloc+j+ipo]  += tmp; //Because size of Jy in Y is b_dim[1]+1.
            Jz[iloc+j]  += crz_p * one_third * ( Sy0[j]*tmp2 + Sy1[j]*tmp3 );
            rho[iloc+j] += charge_weight * Sx1[0]*Sy1[j];
        }
        
    }//end i=0 case
    
    // case i> 0
    for (unsigned int i=1 ; i<5 ; i++) {
        iloc = (i+ipo)*b_dim[1]+jpo;
        tmpJx[0] -= crx_p *  DSx[i-1] * (0.5*DSy[0]);
        Jx[iloc]  += tmpJx[0];
        tmp2 = 0.5*Sx1[i] + Sx0[i];
        tmp3 = 0.5*Sx0[i] + Sx1[i];
        Jz[iloc]  += crz_p * one_third * ( Sy1[0]*tmp3 );
        rho[iloc] += charge_weight * Sx1[i]*Sy1[0];
        tmp = 0;
        tmpY = Sx0[i] + 0.5*DSx[i];
        for (unsigned int j=1 ; j<5 ; j++) {
            tmpJx[j] -= crx_p * DSx[i-1] * (Sy0[j] + 0.5*DSy[j]);
            Jx[iloc+j]  += tmpJx[j];
            tmp -= cry_p * DSy[j-1] * tmpY;
            Jy[iloc+j+i+ipo]  += tmp; //Because size of Jy in Y is b_dim[1]+1.
            Jz[iloc+j]  += crz_p * one_third * ( Sy0[j]*tmp2 + Sy1[j]*tmp3 );
            rho[iloc+j] += charge_weight * Sx1[i]*Sy1[j];
        }
    
    }//i
} // END Project local current densities at dag timestep.


// ---------------------------------------------------------------------------------------------------------------------
//! Project charge : frozen & diagFields timstep (not vectorized)
// ---------------------------------------------------------------------------------------------------------------------
void Projector2D2OrderV::operator() (double* rho, Particles &particles, unsigned int ipart, unsigned int bin, std::vector<unsigned int> &b_dim)
{
    //Warning : this function is used for frozen species only. It is assumed that position = position_old !!!
    
    // -------------------------------------
    // Variable declaration & initialization
    // -------------------------------------
    
    int iloc;
    // (x,y,z) components of the current density for the macro-particle
    double charge_weight = (double)(particles.charge(ipart))*particles.weight(ipart);
    
    // variable declaration
    double xpn, ypn;
    double delta, delta2;
    double Sx1[5], Sy1[5]; // arrays used for the Esirkepov projection method
    
    // Initialize all current-related arrays to zero
    for (unsigned int i=0; i<5; i++) {
        Sx1[i] = 0.;
        Sy1[i] = 0.;
    }
    
    // --------------------------------------------------------
    // Locate particles & Calculate Esirkepov coef. S, DS and W
    // --------------------------------------------------------
    
    // locate the particle on the primal grid at current time-step & calculate coeff. S1
    xpn = particles.position(0, ipart) * dx_inv_;
    int ip = round(xpn);
    delta  = xpn - (double)ip;
    delta2 = delta*delta;
    Sx1[1] = 0.5 * (delta2-delta+0.25);
    Sx1[2] = 0.75-delta2;
    Sx1[3] = 0.5 * (delta2+delta+0.25);
    
    ypn = particles.position(1, ipart) * dy_inv_;
    int jp = round(ypn);
    delta  = ypn - (double)jp;
    delta2 = delta*delta;
    Sy1[1] = 0.5 * (delta2-delta+0.25);
    Sy1[2] = 0.75-delta2;
    Sy1[3] = 0.5 * (delta2+delta+0.25);
    
    // ---------------------------
    // Calculate the total current
    // ---------------------------
    ip -= i_domain_begin + bin +2;
    jp -= j_domain_begin + 2;
    
    for (unsigned int i=0 ; i<5 ; i++) {
        iloc = (i+ip)*b_dim[1]+jp;
        for (unsigned int j=0 ; j<5 ; j++) {
            rho[iloc+j] += charge_weight * Sx1[i]*Sy1[j];
        }
    }//i
} // END Project local current densities frozen.


// ---------------------------------------------------------------------------------------------------------------------
//! Project global current densities : ionization
// ---------------------------------------------------------------------------------------------------------------------
void Projector2D2OrderV::operator() (Field* Jx, Field* Jy, Field* Jz, Particles &particles, int ipart, LocalFields Jion)
{
    Field2D* Jx2D  = static_cast<Field2D*>(Jx);
    Field2D* Jy2D  = static_cast<Field2D*>(Jy);
    Field2D* Jz2D  = static_cast<Field2D*>(Jz);
    
    
    //Declaration of local variables
    int ip, id, jp, jd;
    double xpn, xpmxip, xpmxip2, xpmxid, xpmxid2;
    double ypn, ypmyjp, ypmyjp2, ypmyjd, ypmyjd2;
    double Sxp[3], Sxd[3], Syp[3], Syd[3];
    
    // weighted currents
    double Jx_ion = Jion.x * particles.weight(ipart);
    double Jy_ion = Jion.y * particles.weight(ipart);
    double Jz_ion = Jion.z * particles.weight(ipart);
    
    //Locate particle on the grid
    xpn    = particles.position(0, ipart) * dx_inv_;  // normalized distance to the first node
    ypn    = particles.position(1, ipart) * dy_inv_;  // normalized distance to the first node
    
    // x-primal index
    ip      = round(xpn);                    // x-index of the central node
    xpmxip  = xpn - (double)ip;              // normalized distance to the nearest grid point
    xpmxip2 = xpmxip*xpmxip;                 // square of the normalized distance to the nearest grid point
    
    // x-dual index
    id      = round(xpn+0.5);                // x-index of the central node
    xpmxid  = xpn - (double)id + 0.5;        // normalized distance to the nearest grid point
    xpmxid2 = xpmxid*xpmxid;                 // square of the normalized distance to the nearest grid point
    
    // y-primal index
    jp      = round(ypn);                    // y-index of the central node
    ypmyjp  = ypn - (double)jp;              // normalized distance to the nearest grid point
    ypmyjp2 = ypmyjp*ypmyjp;                 // square of the normalized distance to the nearest grid point
    
    // y-dual index
    jd      = round(ypn+0.5);                // y-index of the central node
    ypmyjd  = ypn - (double)jd + 0.5;        // normalized distance to the nearest grid point
    ypmyjd2 = ypmyjd*ypmyjd;                 // square of the normalized distance to the nearest grid point
    
    Sxp[0] = 0.5 * (xpmxip2-xpmxip+0.25);
    Sxp[1] = (0.75-xpmxip2);
    Sxp[2] = 0.5 * (xpmxip2+xpmxip+0.25);
    
    Sxd[0] = 0.5 * (xpmxid2-xpmxid+0.25);
    Sxd[1] = (0.75-xpmxid2);
    Sxd[2] = 0.5 * (xpmxid2+xpmxid+0.25);
    
    Syp[0] = 0.5 * (ypmyjp2-ypmyjp+0.25);
    Syp[1] = (0.75-ypmyjp2);
    Syp[2] = 0.5 * (ypmyjp2+ypmyjp+0.25);
    
    Syd[0] = 0.5 * (ypmyjd2-ypmyjd+0.25);
    Syd[1] = (0.75-ypmyjd2);
    Syd[2] = 0.5 * (ypmyjd2+ypmyjd+0.25);
    
    ip  -= i_domain_begin;
    id  -= i_domain_begin;
    jp  -= j_domain_begin;
    jd  -= j_domain_begin;
    
    for (unsigned int i=0 ; i<3 ; i++) {
        int iploc=ip+i-1;
        int idloc=id+i-1;
        for (unsigned int j=0 ; j<3 ; j++) {
            int jploc=jp+j-1;
            int jdloc=jd+j-1;
            // Jx^(d,p)
            (*Jx2D)(idloc,jploc) += Jx_ion * Sxd[i]*Syp[j];
            // Jy^(p,d)
            (*Jy2D)(iploc,jdloc) += Jy_ion * Sxp[i]*Syd[j];
            // Jz^(p,p)
            (*Jz2D)(iploc,jploc) += Jz_ion * Sxp[i]*Syp[j];
        }
    }//i


} // END Project global current densities (ionize)


// ---------------------------------------------------------------------------------------------------------------------
//! Project current densities : main projector vectorized
// ---------------------------------------------------------------------------------------------------------------------
void Projector2D2OrderV::operator() (double* Jx, double* Jy, double* Jz, Particles &particles, unsigned int istart, unsigned int iend, std::vector<double> *invgf, std::vector<unsigned int> &b_dim, int* iold, double *deltaold)
{
    // -------------------------------------
    // Variable declaration & initialization
    // -------------------------------------

    int npart_total = particles.size();
    int ipo = iold[0];
    int jpo = iold[1];
    int ipom2 = ipo-2;
    int jpom2 = jpo-2;

    int vecSize = 8;
    int bsize = 5*5*vecSize;

    double bJx[bsize] __attribute__((aligned(64)));

    double Sx0_buff_vect[40] __attribute__((aligned(64)));
    double Sy0_buff_vect[40] __attribute__((aligned(64)));
    double Sx1_buff_vect[40] __attribute__((aligned(64)));
    double Sy1_buff_vect[40] __attribute__((aligned(64)));
    double DSx[40] __attribute__((aligned(64)));
    double DSy[40] __attribute__((aligned(64)));
    double charge_weight[8] __attribute__((aligned(64)));
    double crz_p[8] __attribute__((aligned(64)));

    // Closest multiple of 8 higher or equal than npart = iend-istart.
    int cell_nparts( (int)iend-(int)istart );
    int nbVec = ( iend-istart+(cell_nparts-1)-((iend-istart-1)&(cell_nparts-1)) ) / vecSize;
    if (nbVec*vecSize != cell_nparts)
        nbVec++;

    for (int iivect=0 ; iivect<nbVec; iivect++ ){
        int ivect = vecSize*iivect;

        int np_computed(0);
        if (cell_nparts > vecSize ) {
            np_computed = vecSize;
            cell_nparts -= vecSize;
        }       
        else
            np_computed = cell_nparts;


        #pragma omp simd
        for (unsigned int i=0; i<40; i++) {
            bJx[i] = 0.;
        }

        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){

            // locate the particle on the primal grid at current time-step & calculate coeff. S1
            //                            X                                 //
            double pos = particles.position(0, ivect+ipart+istart) * dx_inv_;
            int cell = round(pos);
            int cell_shift = cell-ipo-i_domain_begin;
            double delta  = pos - (double)cell;
            double delta2 = delta*delta;
            double deltam =  0.5 * (delta2-delta+0.25);
            double deltap =  0.5 * (delta2+delta+0.25);
            delta2 = 0.75 - delta2;
            double m1 = (cell_shift == -1);
            double c0 = (cell_shift ==  0);
            double p1 = (cell_shift ==  1);
            Sx1_buff_vect[          ipart] = m1 * deltam                                                                                  ;
            Sx1_buff_vect[  vecSize+ipart] = c0 * deltam + m1*delta2                                               ;
            Sx1_buff_vect[2*vecSize+ipart] = p1 * deltam + c0*delta2 + m1*deltap;
            Sx1_buff_vect[3*vecSize+ipart] =               p1*delta2 + c0*deltap;
            Sx1_buff_vect[4*vecSize+ipart] =                           p1*deltap;
            // locate the particle on the primal grid at former time-step & calculate coeff. S0
            //                            X                                 //
            delta = deltaold[ivect+ipart+istart];
            delta2 = delta*delta;
            Sx0_buff_vect[          ipart] = 0;
            Sx0_buff_vect[  vecSize+ipart] = 0.5 * (delta2-delta+0.25);
            Sx0_buff_vect[2*vecSize+ipart] = 0.75-delta2;
            Sx0_buff_vect[3*vecSize+ipart] = 0.5 * (delta2+delta+0.25);
            Sx0_buff_vect[4*vecSize+ipart] = 0;
            //optrpt complains about the following loop but not unrolling it actually seems to give better result.
            #pragma unroll
            for (unsigned int i = 0; i < 5 ; i++){
                DSx[i*vecSize+ipart] = Sx1_buff_vect[ i*vecSize+ipart] - Sx0_buff_vect[ i*vecSize+ipart];
            }
            //                            Y                                 //
            pos = particles.position(1, ivect+ipart+istart) * dy_inv_;
            cell = round(pos);
            cell_shift = cell-jpo-j_domain_begin;
            delta  = pos - (double)cell;
            delta2 = delta*delta;
            deltam =  0.5 * (delta2-delta+0.25);
            deltap =  0.5 * (delta2+delta+0.25);
            delta2 = 0.75 - delta2;
            m1 = (cell_shift == -1);
            c0 = (cell_shift ==  0);
            p1 = (cell_shift ==  1);
            Sy1_buff_vect[          ipart] = m1 * deltam                                                                                  ;
            Sy1_buff_vect[  vecSize+ipart] = c0 * deltam + m1*delta2                                               ;
            Sy1_buff_vect[2*vecSize+ipart] = p1 * deltam + c0*delta2 + m1*deltap;
            Sy1_buff_vect[3*vecSize+ipart] =               p1*delta2 + c0*deltap;
            Sy1_buff_vect[4*vecSize+ipart] =                           p1*deltap;
            //                            Y                                 //
            delta = deltaold[ivect+ipart+istart+npart_total];
            delta2 = delta*delta;
            Sy0_buff_vect[          ipart] = 0;
            Sy0_buff_vect[  vecSize+ipart] = 0.5 * (delta2-delta+0.25);
            Sy0_buff_vect[2*vecSize+ipart] = 0.75-delta2;
            Sy0_buff_vect[3*vecSize+ipart] = 0.5 * (delta2+delta+0.25);
            Sy0_buff_vect[4*vecSize+ipart] = 0;

            //optrpt complains about the following loop but not unrolling it actually seems to give better result.
            #pragma unroll
            for (unsigned int i = 0; i < 5 ; i++){
                DSy[i*vecSize+ipart] = Sy1_buff_vect[ i*vecSize+ipart] - Sy0_buff_vect[ i*vecSize+ipart];
            }
            charge_weight[ipart] = (double)(particles.charge(ivect+istart+ipart))*particles.weight(ivect+istart+ipart);
            crz_p[ipart] = charge_weight[ipart]*particles.momentum(2, ivect+istart+ipart)*(*invgf)[ivect+istart+ipart];
        }

        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){
            double crx_p = charge_weight[ipart]*dx_ov_dt;

            double sum[5];
            sum[0] = 0.;
            for (unsigned int k=1 ; k<5 ; k++) {
                sum[k] = sum[k-1]-DSx[(k-1)*vecSize+ipart];
            }
            
            double tmp( crx_p * (0.5*DSy[ipart]) );
            for (unsigned int i=1 ; i<5 ; i++) {
                bJx [(i*5)*vecSize+ipart] = sum[i] * tmp;
            }

            for (unsigned int j=1; j<5 ; j++) {
                double tmp( crx_p * (Sy0_buff_vect[j*vecSize+ipart] + 0.5*DSy[j*vecSize+ipart]) );
                for (unsigned int i=1 ; i<5 ; i++) {
                    bJx [(i*5+j)*vecSize+ipart] = sum[i] * tmp;
                }
            }
        }

        int iloc0 = ipom2*b_dim[1]+jpom2;
        int iloc = iloc0;
        for (unsigned int i=1 ; i<5 ; i++) {
            iloc += b_dim[1];
            #pragma omp simd
            for (unsigned int j=0 ; j<5 ; j++) {
                double tmpJx(0.);
                int ilocal = (i*5+j)*vecSize;
                #pragma unroll
                for (int ipart=0 ; ipart<np_computed; ipart++ ){
                    tmpJx += bJx [ilocal+ipart];
                }
                Jx[iloc+j] += tmpJx;
            }
        }

        for (unsigned int j=0; j<200; j=j+40)
            #pragma omp simd
            for (unsigned int i=0; i<8; i++)
                bJx[j+i] = 0.;
        
        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){
            double cry_p = charge_weight[ipart]*dy_ov_dt;

            double sum[5];
            sum[0] = 0.;
            for (unsigned int k=1 ; k<5 ; k++) {
                sum[k] = sum[k-1]-DSy[(k-1)*vecSize+ipart];
            }

            double tmp( cry_p * (0.5*DSx[ipart]) );
            for (unsigned int j=1 ; j<5 ; j++) {
                bJx [j*vecSize+ipart] = sum[j] * tmp;
            }

            for (unsigned int i=1; i<5 ; i++) {
                double tmp( cry_p * (Sx0_buff_vect[i*vecSize+ipart] + 0.5*DSx[i*vecSize+ipart]) );
                for (unsigned int j=1 ; j<5 ; j++) {
                    bJx [(i*5+j)*vecSize+ipart] = sum[j] * tmp;
                }
            }
        }


        iloc  = iloc0 + ipom2;
        for (unsigned int i=0 ; i<5 ; i++) {
            #pragma omp simd
            for (unsigned int j=1 ; j<5 ; j++) {
                double tmpJy(0.);               
                int ilocal = (i*5+j)*vecSize;
                #pragma unroll
                for (int ipart=0 ; ipart<np_computed; ipart++ ){
                    tmpJy += bJx [ilocal+ipart];
                }
                Jy[iloc+j] += tmpJy;
            }
            iloc += (b_dim[1]+1);
        }

        #pragma omp simd
        for (int ipart=0 ; ipart<np_computed; ipart++ ){
            bJx [ipart] = crz_p[ipart] * one_third * Sx1_buff_vect[ipart] * Sy1_buff_vect[ipart];
            double tmp( crz_p[ipart] * one_third * Sy1_buff_vect[ipart] );
            for (unsigned int i=1 ; i<5 ; i++) {
                bJx [((i)*5)*vecSize+ipart] = tmp * (0.5*Sx0_buff_vect[i*vecSize+ipart] + Sx1_buff_vect[i*vecSize+ipart]);
            }

            tmp = crz_p[ipart] * one_third * Sx1_buff_vect[ipart];
            for (unsigned int j=1; j<5 ; j++) {
                bJx [j*vecSize+ipart] =  tmp * ( 0.5*Sy0_buff_vect[j*vecSize+ipart]* + Sy1_buff_vect[j*vecSize+ipart] );
            }

            for (unsigned int i=1 ; i<5 ; i++) {
                double tmp0( crz_p[ipart] * one_third * (0.5*Sx0_buff_vect[i*vecSize+ipart] + Sx1_buff_vect[i*vecSize+ipart]) );
                double tmp1( crz_p[ipart] * one_third * (0.5*Sx1_buff_vect[i*vecSize+ipart] + Sx0_buff_vect[i*vecSize+ipart]) );
                for (unsigned int j=1; j<5 ; j++) {
                    bJx [((i)*5+j)*vecSize+ipart] = ( Sy0_buff_vect[j*vecSize+ipart]* tmp1 + Sy1_buff_vect[j*vecSize+ipart]* tmp0 );
                }
            }

        } // END ipart (compute coeffs)

        iloc = iloc0;
        for (unsigned int i=0 ; i<5 ; i++) {
            #pragma omp simd
            for (unsigned int j=0 ; j<5 ; j++) {
                double tmpJz(0.);
                int ilocal = (i*5+j)*vecSize;
                #pragma unroll
                for (int ipart=0 ; ipart<np_computed; ipart++ ){
                    tmpJz  +=  bJx [ilocal+ipart];
                }
                Jz[iloc+j]  +=  tmpJz;
            }//i
            iloc += b_dim[1];
        } // ipart
        
    } // ivect

} // END Project vectorized


// ---------------------------------------------------------------------------------------------------------------------
//! Wrapper for projection
// ---------------------------------------------------------------------------------------------------------------------
void Projector2D2OrderV::operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int istart, int iend, int ithread, int scell, int clrw, bool diag_flag, std::vector<unsigned int> &b_dim, int ispec)
{
    if ( istart == iend ) return; //Don't treat empty cells.

    //Independent of cell. Should not be here
    //{   
    std::vector<double> *delta = &(smpi->dynamics_deltaold[ithread]);
    std::vector<double> *invgf = &(smpi->dynamics_invgf[ithread]);
    //}
    int iold[2];
    iold[0] = scell/nprimy+oversize[0];
    iold[1] = (scell%nprimy)+oversize[1];
   
    
    // If no field diagnostics this timestep, then the projection is done directly on the total arrays
    if (!diag_flag){ 
        double* b_Jx =  &(*EMfields->Jx_ )(0);
        double* b_Jy =  &(*EMfields->Jy_ )(0);
        double* b_Jz =  &(*EMfields->Jz_ )(0);
        (*this)(b_Jx , b_Jy , b_Jz , particles,  istart, iend, invgf, b_dim, iold, &(*delta)[0]);
            
    // Otherwise, the projection may apply to the species-specific arrays
    } else {
        int ibin = 0; // Trick to make it compatible for the moment.
        int dim1 = EMfields->dimPrim[1];
        double* b_Jx  = EMfields->Jx_s [ispec] ? &(*EMfields->Jx_s [ispec])(ibin*clrw* dim1   ) : &(*EMfields->Jx_ )(ibin*clrw* dim1   ) ;
        double* b_Jy  = EMfields->Jy_s [ispec] ? &(*EMfields->Jy_s [ispec])(ibin*clrw*(dim1+1)) : &(*EMfields->Jy_ )(ibin*clrw*(dim1+1)) ;
        double* b_Jz  = EMfields->Jz_s [ispec] ? &(*EMfields->Jz_s [ispec])(ibin*clrw* dim1   ) : &(*EMfields->Jz_ )(ibin*clrw* dim1   ) ;
        double* b_rho = EMfields->rho_s[ispec] ? &(*EMfields->rho_s[ispec])(ibin*clrw* dim1   ) : &(*EMfields->rho_)(ibin*clrw* dim1   ) ;
        for (int ipart=istart ; ipart<iend; ipart++ ) {
            //Do not use cells sorting for now : f(ipart) for now, f(istart) laterfor now,
            //(*iold)[ipart       ] = round( particles.position(0, ipart)* dx_inv_ - dt*particles.momentum(0, ipart)*(*invgf)[ipart] * dx_inv_ ) - i_domain_begin ;
            //(*iold)[ipart+nparts] = round( particles.position(1, ipart)* dy_inv_ - dt*particles.momentum(1, ipart)*(*invgf)[ipart] * dy_inv_ ) - j_domain_begin ;
            (*this)(b_Jx , b_Jy , b_Jz ,b_rho, particles,  ipart, (*invgf)[ipart], ibin*clrw, b_dim, iold, &(*delta)[ipart]);
        }
    }
}