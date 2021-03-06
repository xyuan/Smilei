// ----------------------------------------------------------------------------
//! \file Radiation.cpp
//
//! \brief This file contains the class functions for the generic class
//!  Radiation for the particle radiation losses.
//
// ----------------------------------------------------------------------------

#include "Radiation.h"

// -----------------------------------------------------------------------------
//! Constructor for Radiation
// input: simulation parameters & Species index
//! \param params simulation parameters
//! \param species Species index
// -----------------------------------------------------------------------------
Radiation::Radiation(Params& params, Species * species)
{
    // Dimension position
    nDim_ = params.nDim_particle;

    // Time step
    dt    = params.timestep;

    // Inverse of the species mass
    one_over_mass_ = 1./species->mass;

    // Normalized Schwinger Electric Field
    norm_E_Schwinger = params.electron_mass*params.c_vacuum*params.c_vacuum
                     / (params.red_planck_cst* params.reference_angular_frequency_SI);

    // Inverse of norm_E_Schwinger
    inv_norm_E_Schwinger = 1./norm_E_Schwinger;

    // The thread radiated energy is initially null
    radiated_energy = 0;
}

// -----------------------------------------------------------------------------
//! Destructor for Radiation
// -----------------------------------------------------------------------------
Radiation::~Radiation()
{
}

// -----------------------------------------------------------------------------
//! Computation of the quantum parameter for the given
//! thread of particles
//! \param Particles class containg the particle property arrays
//! \param smpi class for mpi parameters
//! \param istart      Index of the first particle
//! \param iend        Index of the last particle
//! \param ithread     Thread index
// -----------------------------------------------------------------------------
void Radiation::compute_thread_chipa(Particles &particles,
        SmileiMPI* smpi,
        int istart,
        int iend,
        int ithread, int ipart_ref)
{
    // _______________________________________________________________
    // Parameters
    std::vector<double> *Epart = &(smpi->dynamics_Epart[ithread]);
    std::vector<double> *Bpart = &(smpi->dynamics_Bpart[ithread]);

    int nparts = Epart->size()/3;
    double* Ex = &( (*Epart)[0*nparts] );
    double* Ey = &( (*Epart)[1*nparts] );
    double* Ez = &( (*Epart)[2*nparts] );
    double* Bx = &( (*Bpart)[0*nparts] );
    double* By = &( (*Bpart)[1*nparts] );
    double* Bz = &( (*Bpart)[2*nparts] );

    // Charge divided by the square of the mass
    double charge_over_mass2;

    // 1/mass^2
    double one_over_mass_square = pow(one_over_mass_,2.);

    // Temporary Lorentz factor
    double gamma;

    // Momentum shortcut
    double* momentum[3];
    for ( int i = 0 ; i<3 ; i++ )
        momentum[i] =  &( particles.momentum(i,0) );

    // Charge shortcut
    short* charge = &( particles.charge(0) );

    // Quantum parameter
    double* chi = &( particles.chi(0));

    // _______________________________________________________________
    // Computation

    #pragma omp simd
    for (int ipart=istart ; ipart<iend; ipart++ )
    {
        charge_over_mass2 = (double)(charge[ipart])*one_over_mass_square;

        // Gamma
        gamma = sqrt(1.0 + momentum[0][ipart]*momentum[0][ipart]
                         + momentum[1][ipart]*momentum[1][ipart]
                         + momentum[2][ipart]*momentum[2][ipart]);

        // Computation of the Lorentz invariant quantum parameter
        chi[ipart] = Radiation::compute_chipa(charge_over_mass2,
                 momentum[0][ipart],momentum[1][ipart],momentum[2][ipart],
                 gamma,
                 (*(Ex+ipart-ipart_ref)),(*(Ey+ipart-ipart_ref)),(*(Ez+ipart-ipart_ref)),
                 (*(Bx+ipart-ipart_ref)),(*(By+ipart-ipart_ref)),(*(Bz+ipart-ipart_ref)) );

    }
}
