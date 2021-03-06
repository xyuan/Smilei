#ifndef PROJECTOR3D2ORDER_H
#define PROJECTOR3D2ORDER_H

#include "Projector3D.h"


class Projector3D2Order : public Projector3D {
public:
    Projector3D2Order(Params&, Patch* patch);
    ~Projector3D2Order();

    //! Project global current densities (EMfields->Jx_/Jy_/Jz_)
    inline void operator() (double* Jx, double* Jy, double* Jz, Particles &particles, unsigned int ipart, double invgf, int* iold, double* deltaold);
    //! Project global current densities (EMfields->Jx_/Jy_/Jz_/rho), diagFields timestep
    inline void operator() (double* Jx, double* Jy, double* Jz, double* rho, Particles &particles, unsigned int ipart, double invgf, int* iold, double* deltaold);

    //! Project global current charge (EMfields->rho_ , J), for initialization and diags
    void operator() (double* rhoj, Particles &particles, unsigned int ipart, unsigned int type, std::vector<unsigned int> &b_dim) override final;

    //! Project global current densities if Ionization in Species::dynamics,
    void operator() (Field* Jx, Field* Jy, Field* Jz, Particles &particles, int ipart, LocalFields Jion) override final;

    //!Wrapper
    void operator() (ElectroMagn* EMfields, Particles &particles, SmileiMPI* smpi, int istart, int iend, int ithread, int ibin, int clrw, bool diag_flag, bool is_spectral, std::vector<unsigned int> &b_dim, int ispec, int ipart_ref = 0) override final;

    void project_susceptibility(ElectroMagn* EMfields, Particles &particles, double species_mass, SmileiMPI* smpi, int istart, int iend,  int ithread, int ibin, std::vector<unsigned int> &b_dim, int ipart_ref = 0) override final;

private:
    double one_third;
    double dt, dts2, dts4;
};

#endif

