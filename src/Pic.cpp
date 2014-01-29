////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                                                ////
////                                                                                                                ////
////                                   PARTICLE-IN-CELL CODE SMILEI                                                 ////
////                    Simulation of Matter Irradiated by Laser at Extreme Intensity                               ////
////                                                                                                                ////
////                          Cooperative OpenSource Object-Oriented Project                                        ////
////                                      from the Plateau de Saclay                                                ////
////                                          started January 2013                                                  ////
////                                                                                                                ////
////                                                                                                                ////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "Pic.h"
#include "PicParams.h"
#include "InputData.h"
#include "DiagParams.h"

#include "SmileiMPIFactory.h"
#include "SmileiIOFactory.h"

#include "SpeciesFactory.h"
#include "ElectroMagnFactory.h"
#include "InterpolatorFactory.h"
#include "ProjectorFactory.h"

#include <ctime>
#include <cstdlib>
#include <iostream>

#include "Diagnostic.h"
#include "DiagnosticProbe0D.h"

#include <unistd.h>

using namespace std;


// ------------------------------------------------------------------------------------------------------------------ //
//                                                   MAIN CODE
// ------------------------------------------------------------------------------------------------------------------ //
int main (int argc, char* argv[])
{
	std::cout.setf( std::ios::fixed, std:: ios::floatfield ); // floatfield set to fixed
	
	// Define 2 MPI environment :
	//  - smpiData : to broadcast input data, unknown geometry
	//  - smpi (defined later) : to compute/exchange data, specific to a geometry
 	SmileiMPI *smpiData= new SmileiMPI(&argc, &argv );
	
	// -------------------------
	// Simulation Initialization
	// -------------------------
	

	// Check for run flags
	
	char ch;
	DEBUGEXEC(debug_level=0);
	
	string dirname("");
	while ((ch = getopt(argc, argv, "d:D:")) != -1) {
		if (ch=='d') {
			RELEASEEXEC(WARNING("In release mode debug option has no meaning, please recompile in debug mode"));
			DEBUGEXEC(std::stringstream iss(optarg);iss >> std::boolalpha >> debug_level;)
		} else if (ch=='D') {
			dirname=string(optarg);
		}
	}
	
	argc -= optind;
	argv += optind;
	
	// Check for namelist (input file)
	if (argc<1) ERROR("No namelists given!");
	string namelist=argv[0];
	
	// Send information on current simulation
	if ( smpiData->isMaster() ) startingMessage(namelist);
	
	// Parse the namelist file (no check!)
	InputData input_data;
	if ( smpiData->isMaster() ) input_data.parseFile(namelist);
	
	if (! dirname.empty()) {
		if (chdir(dirname.c_str())!=0) {
			ERROR("Directory " << dirname << " not found");
		}
	}

	smpiData->bcast( input_data );
	input_data.parseStream();
		
	// this will do the randomization (changing the seed for all processes)
	unsigned long seedTime=0;
	if (!input_data.extract("random_seed",seedTime)) {
		RELEASEEXEC(seedTime=time(NULL));
		input_data.addVar("random_seed",seedTime);
	}
	srand(seedTime+smpiData->getRank());
	
	input_data.write(getFileWithoutExt(namelist)+".parsed");
	
	// Read simulation parameters
	PicParams params(input_data);
	smpiData->init(params);
	DiagParams diag_params(input_data,params);
	
	for (int i=0;i<smpiData->getSize(); i++) {
		if (i==smpiData->getRank()) {
			params.print();
		}
		smpiData->barrier();
	}

	// Geometry known, MPI environment specified
	SmileiMPI* smpi = SmileiMPIFactory::create(params, smpiData);

	SmileiIO*  sio  = SmileiIOFactory::create(params, smpi);

	
	// -------------------------------------------
	// Declaration of the main objects & operators
	// -------------------------------------------
	
    
	// ----------------------------------------------------------------------------
	// Initialize the electromagnetic fields and interpolation-projection operators
	// according to the simulation geometry
	// ----------------------------------------------------------------------------
	// object containing the electromagnetic fields (virtual)
	ElectroMagn* EMfields = ElectroMagnFactory::create(params, smpi);
	
	// interpolation operator (virtual)
	Interpolator* Interp = InterpolatorFactory::create(params, smpi);
    
	// projection operator (virtual)
	Projector* Proj = ProjectorFactory::create(params, smpi);

    
    // ------------------------------------------------------------------------------------
	// Initialize the vecSpecies object containing all information of the different Species
	// ------------------------------------------------------------------------------------
	// vector of Species (virtual)
	vector<Species*> vecSpecies = SpeciesFactory::createVector(params, smpi);
	
    

    // ----------------------------------------------------------------------------
	// Create diagnostics
	// ----------------------------------------------------------------------------
	Diagnostic diags(&params,&diag_params, smpi, Interp);	

	// -----------------------------------
	// Initialize the electromagnetic fields
	// -----------------------------------   
	//!\todo{Check & describe what is done here (MG)}
	// Init rho by pro all particles of subdomain -> local stuff
	EMfields->initRhoJ(vecSpecies, Proj);
    
    // Initializing the total charge & current densities
    smpi->sumRhoJ( EMfields );
    
	//! \todo{FalseNot //, current algorithm is instrinsicaly sequential}
	
    EMfields->solvePoisson(smpi);//champs->initMaxwell();$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	
    smpi->barrier();

    
	// ------------------------------------------------------------------------
	// Initialize the simulation times time_prim at n=0 and time_dual at n=-1/2
	// ------------------------------------------------------------------------

	// time at integer time-steps (primal grid)
	double time_prim = 0.;
	// time at half-integer time-steps (dual grid)
	double time_dual = 0.5 * params.timestep;
    
    // run diagnostics at time-step 0
    diags.runAllDiags(0, EMfields, vecSpecies);
    // temporary EM fields dump in Fields.h5
    sio->writeAllFieldsSingleFileTime( EMfields, 0 );
    // temporary particle dump at time 0
	sio->writePlasma( vecSpecies, 0., smpi );
	
    
	// ------------------------------------------------------------------
	//                     HERE STARTS THE PIC LOOP
	// ------------------------------------------------------------------
	if ( smpi->isMaster() ) MESSAGE(0,"Time-Loop is started: number of time-steps n_time =" << params.n_time);
	// t1-t0  = elapsed time in simulation time loop
	double t0, t1;
	t0 = MPI_Wtime();
    
	for (unsigned int itime=1 ; itime <= params.n_time ; itime++) {

		// calculate new times
		// -------------------
		time_prim += params.timestep;
		time_dual += params.timestep; 
		
		// send message at given time-steps
		// --------------------------------

		if ( (itime % diag_params.print_every == 0) &&  ( smpi->isMaster() ) )
			MESSAGE(1,"Time (dual)= " << time_dual << " it = " << itime  << " / " << params.n_time);
		
        
		// put density and currents to 0 + save former density
		// ---------------------------------------------------
		EMfields->restartRhoJ();
        
		
		// apply the PIC method
		// --------------------
		// for all particles of all species (see dunamic in Species.cpp)
		// (1) interpolate the fields at the particle position
		// (2) move the particle
		// (3) calculate the currents (charge conserving method)
		for (unsigned int ispec=0 ; ispec<params.n_species; ispec++) {
			vecSpecies[ispec]->dynamics(time_dual, ispec, EMfields, Interp, Proj, smpi);
			smpi->exchangeParticles(vecSpecies[ispec], ispec, &params);
			if (params.nDim_field == 1)//$$$$$$$$$$$$$$$$$$$$$$$$
				vecSpecies[ispec]->sort_part(params.cell_length[params.nDim_particle-1]);//$$$$$$$$$$$$$$$$$$$$$
		}
		smpi->sumRhoJ( EMfields );
        EMfields->computeTotalRhoJ();
		
		// solve Maxwell's equations
		EMfields->solveMaxwell(time_dual, smpi); //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
		
        // call the various diagnostics
		// ----------------------------
		
        // run all diagnostics
		diags.runAllDiags(itime, EMfields, vecSpecies);
        
        // temporary EM fields dump in Fields.h5
		if  ((diag_params.fieldDump_every != 0) && (itime % diag_params.fieldDump_every == 0))
			sio->writeAllFieldsSingleFileTime( EMfields, itime );
        
        // temporary particles dump (1 HDF5 file per process)
		if  ((diag_params.particleDump_every != 0) && (itime % diag_params.particleDump_every == 0))
            sio->writePlasma( vecSpecies, time_dual, smpi );

	}//END of the time loop	
	
	smpi->barrier();
	t1 = MPI_Wtime();
	if ( smpi->isMaster() ) MESSAGE(0, "End time loop, time dual = " << time_dual);
	if ( smpi->isMaster() ) MESSAGE(0, "Time in time loop : " << t1-t0 );
	// ------------------------------------------------------------------
	//                      HERE ENDS THE PIC LOOP
	// ------------------------------------------------------------------
	
	
	// ------------------------------------------------------------------
	//                      Temporary validation diagnostics
	// ------------------------------------------------------------------
	
     // temporary EM fields dump in Fields.h5
    if  ( (diag_params.fieldDump_every != 0) && (params.n_time % diag_params.fieldDump_every != 0) )
		sio->writeAllFieldsSingleFileTime( EMfields, params.n_time );
    
	// temporary particles dump (1 HDF5 file per process)
    if  ( (diag_params.particleDump_every != 0) && (params.n_time % diag_params.particleDump_every != 0) )
        sio->writePlasma( vecSpecies, time_dual, smpi );
	
/*	//EMfields->initRho(vecSpecies, Proj);
	//smpi->sumRho( EMfields );
	
	//EMfields->dump(&params);  	// Sequential results, 1 file per process
	if (params.nDim_field == 1) { // If 1D
		//! \todo{Not //, processes write sequentially to validate. OK in 1D}
		//smpi->writeFields( EMfields );
		// Using HDF5, both (sio, smpi) while python tools not updated
		sio->writeFields( EMfields );
	}
	else { // If 2D
		sio->writeFields( EMfields );
		//sio->writeFieldsPP( EMfields, time_dual, smpi->getRank() );
	}
*/
	
	
	// ------------------------------
	//  Cleanup & End the simulation
	// ------------------------------
	delete Proj;
	delete Interp;
	delete EMfields;
    diags.closeAll();
    
	//for (unsigned int ispec=0 ; ispec<vecSpecies.size(); ispec++) delete vecSpecies[ispec];
	//vecSpecies.clear();
	
	delete sio;
	if ( smpi->isMaster() ) {
		MESSAGE("--------------------------------------------------------------------------------");
		MESSAGE("END " << namelist);
		MESSAGE("--------------------------------------------------------------------------------");
	}
	delete smpi;
	delete smpiData;
	return 0;
    
}//END MAIN 

void startingMessage(std::string inputfile) {
	MESSAGE("--------------------------------------------------------------------------------");
	MESSAGE(" Version  : " << __VERSION DEBUGEXEC(<< " DEBUG") << " Compiled : " << __DATE__ << " " << __TIME__);
	MESSAGE("--------------------------------------------------------------------------------");
	MESSAGE(" Namelist : " << inputfile);
	MESSAGE("--------------------------------------------------------------------------------");
}


string getFileWithoutExt(const string& s) {
	size_t i = s.rfind('.', s.length( ));
	if (i != string::npos) {
		return(s.substr(0, i));
	}	
	return("");
}

