Post-process
------------

This page describes the usage of the python module ``happi`` for extracting, viewing
and post-processing simulation data. First, you need to :ref:`install happi <installModule>`.

The module can be imported directly in *python*::

  import happi


----

Open a simulation
^^^^^^^^^^^^^^^^^^^

In a *python* command line (or script), call the following function to open
your :program:`Smilei` simulation. Note that several simulations can be opened at once,
as long as they correspond to several :ref:`restarts <Checkpoints>` of the same simulation.

.. py:method:: happi.Open(results_path=".", show=True, reference_angular_frequency_SI=None, verbose=True)

  * ``results_path``: path or list of paths to the directory-ies
    where the results of the simulation-s are stored. It can also contain wildcards,
    such as ``*`` and ``?`` in order to include several simulations at once.

  * ``reference_angular_frequency_SI``: overrides the value of the simulation parameter
    :py:data:`reference_angular_frequency_SI`, in order to re-scale units.

  * ``show``: if ``False``, figures will not plot on screen. Make sure that
    you have not loaded another simulation or the matplotlib package. You may need to
    restart python.

  * ``verbose``: if ``False``, less information is printed while post-processing.

  * ``scan``: if ``False``, HDF5 output files are not scanned initially.


**Returns:** An object containing various methods to extract and manipulate the simulation
  outputs, as described below.

**Example**::

  S = happi.Open("path/to/my/results")


Once a simulation is opened, several methods are available to find information on the
namelist or open various diagnostics. Checkout the namelist documentation to find out
which diagnostics are included in Smilei: :ref:`scalars <DiagScalar>`,
:ref:`fields <DiagFields>`, :ref:`probes <DiagProbe>`,
:ref:`particle binning <DiagParticleBinning>`, :ref:`trajectories <DiagTrackParticles>`
and :ref:`performances <DiagPerformances>`.

----

Extract namelist information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once a simulation is opened as shown above, you can access the content of the namelist
using the attribute ``namelist``::

  S = happi.Open("path/to/my/results") # Open a simulation
  print(S.namelist.Main.timestep)   # print the timestep
  print(S.namelist.Main.geometry)   # print the simulation dimensions

All the variables defined in the original namelist are copied into this variable.

Concerning components like :ref:`Species`, :ref:`ExternalField` or :ref:`DiagProbe`, of which
several instances may exist, you can directly iterate over them::

  for species in S.namelist.Species:
      print("species "+species.name+" has mass "+str(species.mass))

You can also access to a specific component by referencing its number::

  F = S.namelist.ExternalField[0]  # get the first external field
  print("An external field "+F.field+" was applied")

In the case of the species, you can also obtain a given species by its name::

  species = S.namelist.Species["electron1"]
  print("species "+species.name+" has mass "+str(species.mass))


----

Open a Scalar diagnostic
^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Scalar(scalar=None, timesteps=None, units=[""], data_log=False, **kwargs)

  * ``scalar``: The name of the scalar.
     | If not given, then a list of available scalars is printed.
  * ``timesteps``: The requested timestep(s).
     | If omitted, all timesteps are used.
     | If one number  given, the nearest timestep available is used.
     | If two numbers given, all the timesteps in between are used.
  * ``units``: A unit specification (see :ref:`units`)
  * ``data_log``:
     | If ``True``, then :math:`\log_{10}` is applied to the output.
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.Scalar("Utot")

----

Open a Field diagnostic
^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Field(diagNumber=None, field=None, timesteps=None, subset=None, average=None, units=[""], data_log=False, moving=False, **kwargs)

  * ``timesteps``, ``units``, ``data_log``: same as before.
  * ``diagNumber``: The number of the fields diagnostic
     | If not given, then a list of available diagnostic numbers is printed.
  * ``field``: The name of a field (``"Ex"``, ``"Ey"``, etc.)
     | If not given, then a list of available fields is printed.
     | The string can also be an operation between several fields, such as ``"Jx+Jy"``.
  * ``subset``: A selection of coordinates to be extracted.
     | Syntax 1: ``subset = { axis : location, ... }``
     | Syntax 2: ``subset = { axis : [start, stop] , ... }``
     | Syntax 3: ``subset = { axis : [start, stop, step] , ... }``
     | ``axis`` must be ``"x"``, ``"y"`` or ``"z"``.
     | Only the data within the chosen axes' selections is extracted.
     | **WARNING:** THE VALUE OF ``step`` IS A NUMBER OF CELLS.
     | Example: ``subset = {"y":[10, 80, 4]}``
  * ``average``: A selection of coordinates on which to average.
     | Syntax 1: ``average = { axis : "all", ... }``
     | Syntax 2: ``average = { axis : location, ... }``
     | Syntax 3: ``average = { axis : [start, stop] , ... }``
     | ``axis`` must be ``"x"``, ``"y"`` or ``"z"``.
     | The chosen axes will be removed:
     | - With syntax 1, an average is performed over all the axis.
     | - With syntax 2, only the bin closest to ``location`` is kept.
     | - With syntax 3, an average is performed from ``start`` to ``stop``.
     | Example: ``average = {"x":[4,5]}`` will average for :math:`x` within [4,5].
  * ``moving``: If ``True``, plots will display the X coordinates evolving according to the
    :ref:`moving window<movingWindow>`
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

  In the case of a spectral cylindrical geometry (``3drz``), additional argument are
  available. You must choose one of ``theta`` or ``build3d``, defined below, in order
  to construct fields from their complex angular Fourier modes. In addition, the ``modes``
  argument is optional.

  * ``theta``: An angle (in radians)
     | Calculates the field in a plane passing through the :math:`r=0` axis
     | and making an angle ``theta`` with the :math:`xz` plane.
  * ``build3d``: A list of three *ranges*
     | Calculates the field interpolated in a 3D :math:`xyz` grid.
     | Each *range* is a list ``[start, stop, step]`` indicating the beginning,
     | the end and the step of this grid.
  * ``modes``: An integer or a list of integers
     | Only these modes numbers will be used in the calculation

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.Field(0, "Ex", average = {"x":[4,5]})


----

Open a Probe diagnostic
^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Probe(probeNumber=None, field=None, timesteps=None, subset=None, average=None, units=[""], data_log=False, **kwargs)

  * ``timesteps``, ``units``, ``data_log``: same as before.
  * ``probeNumber``: number of the probe (the first one has number 0).
     | If not given, a list of available probes is printed.
  * ``field``: name of the field (``"Bx"``, ``"By"``, ``"Bz"``, ``"Ex"``, ``"Ey"``, ``"Ez"``, ``"Jx"``, ``"Jy"``, ``"Jz"`` or ``"Rho"``).
     | If not given, then a list of available fields is printed.
     | The string can also be an operation between several fields, such as ``"Jx+Jy"``.
  * ``subset`` and ``average`` are very similar to those of :py:meth:`Field`, but they can only have the axes: ``"axis1"``, ``"axis2"`` and ``"axis3"``.
    For instance, ``average={"axis1":"all"}``. Note that the axes are not necessarily :math:`x`, :math:`y` or :math:`z` because the probe mesh is arbitrary.
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.Probe(0, "Ex")


----

Open a ParticleBinning diagnostic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: ParticleBinning(diagNumber=None, timesteps=None, subset=None, sum=None, units=[""], data_log=False, **kwargs)

  * ``timesteps``, ``units``, ``data_log``: same as before.
  * ``diagNumber``: number of the particle binning diagnostic (starts at 0).
     | If not given, a list of available diagnostics is printed.
     | It can also be an operation between several diagnostics.
     | For example, ``"#0/#1"`` computes the division by diagnostics 0 and 1.
  * ``subset`` is similar to that of :py:meth:`Field`, although the axis must be one of
     ``"x"``, ``"y"``, ``"z"``, ``"px"``, ``"py"``, ``"pz"``, ``"p"``, ``"gamma"``, ``"ekin"``, ``"vx"``, ``"vy"``, ``"vz"``, ``"v"`` or ``"charge"``.

     **WARNING:** With the syntax ``subset={axis:[start, stop, step]}``, the value of ``step``
     is a number of bins.
  * ``sum``: a selection of coordinates on which to sum the data.
     | Syntax 1: ``sum = { axis : "all", ... }``
     | Syntax 2: ``sum = { axis : location, ... }``
     | Syntax 3: ``sum = { axis : [begin, end] , ... }``

     ``axis`` must be ``"x"``, ``"y"``, ``"z"``, ``"px"``, ``"py"``, ``"pz"``, ``"p"``, ``"gamma"``, ``"ekin"``, ``"vx"``, ``"vy"``, ``"vz"``, ``"v"`` or ``"charge"``.

     | The chosen axes will be removed:
     | - With syntax 1, a sum is performed over all the axis.
     | - With syntax 2, only the bin closest to ``location`` is kept.
     | - With syntax 3, a sum is performed between ``begin`` and ``end``.
     | Example: ``sum={"x":[4,5]}`` will sum all the data for x within [4,5].
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.ParticleBinning(1)



----

Open a Screen diagnostic
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Screen(diagNumber=None, timesteps=None, subset=None, sum=None, units=[""], data_log=False, **kwargs)

  * ``timesteps``, ``units``, ``data_log``: same as before.
  * ``diagNumber``: number of the screen diagnostic (the first one has number 0).
     | If not given, a list of available screen diagnostics is printed.
     | It can also be an operation between several Screen diagnostics.
     | For example, ``"#0/#1"`` computes the division by diagnostics 0 and 1.
  * ``subset`` and ``sum``: identical to that of ParticleBinning diagnostics.
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.Screen(0)



----

Open a TrackParticles diagnostic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: TrackParticles(species=None, select="", axes=[], timesteps=None, sort=True, length=None, units=[""], **kwargs)

  * ``timesteps``, ``units``: same as before.
  * ``species``: the name of a tracked-particle species.
     | If omitted, a list of available tracked-particle species is printed.
  * ``select``: Instructions for selecting particles among those available.
    A detailed explanation is provided below
  * ``axes``: A list of axes for plotting the trajectories or obtaining particle data.
     | Each axis is one of the :py:data:`attributes` defined in the namelist.
     | **Example:** ``axes = ["x"]`` corresponds to :math:`x` versus time.
     | **Example:** ``axes = ["x","y"]`` correspond to 2-D trajectories.
     | **Example:** ``axes = ["x","px"]`` correspond to phase-space trajectories.
  * ``sort``: If ``False``, the particles are not sorted by ID. This can save significant
    time, but prevents plotting, exporting to VTK, and the ``select`` argument. Only
    ``getData()`` is available in this mode. Read :doc:`this <ids>` for more information
    on particle IDs.
  * ``length``: The length of each plotted trajectory, in number of timesteps.
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

**Example**::

  S = happi.Open("path/to/my/results")
  Diag = S.TrackParticles("electrons", axes=["px","py"])


.. rubric:: Detailed explanation of the ``select`` parameter

| Say ``times`` is a condition on timesteps ``t``, for instance ``t>50``.
| Say ``condition`` is a condition on particles properties  (``x``, ``y``, ``z``, ``px``, ``py``, ``pz``), for instance ``px>0``.

* | **Syntax 1:** ``select="any(times, condition)"``
  | Selects particles satisfying ``condition`` for at least one of the ``times``.
  | For example, ``select="any(t>0, px>1.)"`` selects those reaching :math:`p_x>1` at some point.

* | **Syntax 2:** ``select="all(times, condition)"``
  | Selects particles satisfying ``condition`` at all ``times``.
  | For example, ``select="all(t<40, px<1)"`` selects those having :math:`p_x<1` until timestep 40.

* | **Syntax 3:** ``select=[ID1, ID2, ...]``
  | Selects the provided particle IDs.

* | It is possible to make logical operations: ``+`` is *OR*; ``*`` is *AND*; ``~`` is *NOT*.
  | For example, ``select="any((t>30)*(t<60), px>1) + all(t>0, (x>1)*(x<2))"``



----

Open a Performances diagnostic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The post-processing of the *performances* diagnostic may be achieved in three different
modes: ``raw``, ``map``, or ``histogram``, described further below. You must choose one
and only one mode between those three.

.. py:method:: Performances(raw=None, map=None, histogram=None, timesteps=None, units=[""], data_log=False, species=None, **kwargs)

  * ``timesteps``, ``units``, ``data_log``: same as before.
  * ``raw`` : The name of a quantity, or an operation between them (see quantities below).
    The requested quantity is listed for each process.
  * ``map`` : The name of a quantity, or an operation between them (see quantities below).
    The requested quantity is mapped vs. space coordinates (1D and 2D only).
  * ``histogram`` : the list ``["quantity", min, max, nsteps]``.
    Makes a histogram of the requested quantity between ``min`` an ``max``, with ``nsteps`` bins.
    The ``"quantity"`` may be an operation between the quantities listed further below.
  * Other keyword arguments (``kwargs``) are available, the same as the function :py:func:`plot`.

There are two kinds of quantities. Some are computed at the MPI process level
and are therefore averaged other the patches.
Some are provided at the patch level.
In the latter case, ``patch_information = True`` has to be put in the namelist.

  **WARNING**: The patch quantities are only compatible with the ``raw`` mode in 3D.
  The result is the patch matrix with the quantity on each patch

**Available quantities at the MPI level**:

  * ``hindex``                     : the starting index of each proc in the hilbert curve
  * ``number_of_cells``            : the number of cells in each proc
  * ``number_of_particles``        : the number of particles in each proc (except frozen ones)
  * ``number_of_frozen_particles`` : the number of frozen particles in each proc
  * ``total_load``                 : the `load` of each proc (number of particles and cells with cell_load coefficient)
  * ``timer_global``               : global simulation time (only available for proc 0)
  * ``timer_particles``            : time spent computing particles by each proc
  * ``timer_maxwell``              : time spent solving maxwell by each proc
  * ``timer_densities``            : time spent projecting densities by each proc
  * ``timer_collisions``           : time spent computing collisions by each proc
  * ``timer_movWindow``            : time spent handling the moving window by each proc
  * ``timer_loadBal``              : time spent balancing the load by each proc
  * ``timer_syncPart``             : time spent synchronzing particles by each proc
  * ``timer_syncField``            : time spent synchronzing fields by each proc
  * ``timer_syncDens``             : time spent synchronzing densities by each proc
  * ``timer_diags``                : time spent by each proc calculating and writing diagnostics
  * ``timer_total``                : the sum of all timers above (except timer_global)

**Available quantities at the patch level**:
  * ``mpi_rank``                   : the MPI rank that contains the current patch
  * ``vecto``                      : the mode of the specified species in the current patch
    (vectorized of scalar) when the adaptive mode is activated. Here the ``species`` argument has to be specified.

  **WARNING**: The timers ``loadBal`` and ``diags`` span parts of the code where *global*
  communications take place. This means they will include time spent doing no calculations,
  waiting for  other processes. The timers ``syncPart``, ``syncField`` and ``syncDens``
  contain *proc-to-proc* communications, which also represents some waiting time.

**Example**: performance diagnostic at the MPI level::

  S = happi.Open("path/to/my/results")
  Diag = S.Performances(map="total_load")

**Example**: performance diagnostic at the patch level::

  S = happi.Open("path/to/my/results")
  Diag = S.Performances(raw="vecto", species="electron")

----

.. _units:

Specifying units
^^^^^^^^^^^^^^^^

By default, all the diagnostics data is in code units (see :doc:`units`).

To change the units, all the methods :py:meth:`Scalar() <Scalar>`,
:py:meth:`Field() <Field>`, :py:meth:`Probe() <Probe>`,
:py:meth:`ParticleBinning() <ParticleBinning>` and
:py:meth:`TrackParticles() <TrackParticles>` support a ``units`` argument.
It has three different syntaxes:

1. **A list**, for example ``units = ["um/ns", "feet", "W/cm^2"]``

   In this case, any quantity found to be of the same dimension as one of these units
   will be converted.

2. **A dictionary**, for example ``units = {"x":"um", "y":"um", "v":"Joule"}``

   In this case, we specify the units separately for axes ``x`` and ``y``, and for the
   data values ``v``.

3. **A** ``Units`` **object**, for example ``units = happi.Units("um/ns", "feet", x="um")``

   This version combines the two previous ones.

.. rubric:: Requirements for changing units

* The `Pint module <https://pypi.python.org/pypi/Pint/>`_.
* To obtain units in a non-normalized system (e.g. SI), the simulation must have the
  parameter :py:data:`reference_angular_frequency_SI` set to a finite value.
  Otherwise, this parameter can be set during post-processing as an argument to the
  :py:meth:`happi.Open` function.


----

Units of ParticleBinning and Screen
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Most of the data output from the ``ParticleBinning`` and ``Screen`` diagnostics is
proportional to the macro-particle statistical weights, as stated by their
:py:data:`deposited_quantity` argument. These weights :ref:`are defined as <Weights>`
the species density divided by the number of macro-particles of this species in the
cell where the particle is born. Consequently, weights are in units of the number
density :math:`N_r` (see :doc:`units`), and the sum of all macro-particle weights in
one cell corresponds to the local species density.

However, the ``DiagParticleBinning`` and ``DiagScreen`` diagnostics do not necessarily
sum the weights in one cell: they calculate a sum over a *bin* that can be of any size,
and in any space (phase-space, etc). The data written in the outputs
``ParticleBinning*.h5`` and ``Screen*.h5`` is thus in units of :math:`N_r`, but does not
always represent the actual plasma density.

In ``happi``, the post-processing takes into account these features in order for outputs
to represent the actual plasma density. For instance, if your ``DiagParticleBinning``
has two axes ``x`` and ``y``, the output will represent the real plasma density. Another
example: if your ``DiagParticleBinning`` has two axes ``x`` and ``px``, the output
will be in units of :math:`N_r/P_r` and represents the real plasma density per unit of
:math:`p_x`.

Let us briefly explain how ``happi`` applies the correction. The idea is that the output
has to be divided by the number of cells *relevant* to each bin.

* The output is divided by the total number of cells in the simulation

* For each axis of the diagnostic :py:data:`axes`:

  * If the axis is one of ``"x"``, ``"y"`` or ``"z"``:

    * Multiply the outputs by the simulation length along that axis.
    * If the axis is included in a ``subset`` or a ``sum``: divide by the corresponding *length*.
    * Otherwise, divide by the length of each bin.

  * Otherwise:

    * If the axis is included in a ``subset`` or a ``sum``: do nothing.
    * Otherwise, divide by the length of each bin.

As a final note, remember that the :py:data:`axes` of a diagnostic may be a *python function*.
In this case, if the function represents some spatial axis, then happi cannot know how to
apply the correction. The user has to figure it out.


----

Obtain the data
^^^^^^^^^^^^^^^

.. py:method:: Scalar.getData( timestep=None )
               Field.getData( timestep=None )
               Probe.getData( timestep=None )
               ParticleBinning.getData( timestep=None )
               Screen.getData( timestep=None )
               TrackParticles.getData( timestep=None )

  Returns a list of the data arrays (one element for each timestep requested).
  In the case of ``TrackParticles``, this method returns a dictionary containing one
  entry for each axis, and if ``sort==True``, these entries are included inside an entry
  for each timestep.

  * ``timestep``, if specified, is the only timestep number that is read and returned.

  **Example**::

      S = happi.Open("path/to/results") # Open the simulation
      Diag = S.Field(0, "Ex")       # Open Ex in the first Field diag
      result = Diag.getData()       # Get list of Ex arrays (one for each time)


.. py:method:: Scalar.getTimesteps()
               Field.getTimesteps()
               Probe.getTimesteps()
               ParticleBinning.getTimesteps()
               Screen.getTimesteps()
               TrackParticles.getTimesteps()

  Returns a list of the timesteps requested.


.. py:method:: Scalar.getTimes()
               Field.getTimes()
               Probe.getTimes()
               ParticleBinning.getTimes()
               Screen.getTimes()
               TrackParticles.getTimes()

  Returns the list of the times requested.
  By default, times are in the code's units, but are converted to the diagnostic's
  units defined by the ``units`` argument, if provided.


.. py:method:: Scalar.getAxis( axis )
               Field.getAxis( axis )
               Probe.getAxis( axis )
               ParticleBinning.getAxis( axis )
               Screen.getAxis( axis )

  Returns the list of positions of the diagnostic data along the requested axis.
  If the axis is not available, returns an empty list.
  By default, axis positions are in the code's units, but are converted to
  the diagnostic's units defined by the ``units`` argument, if provided.

  * ``axis``: the name of the requested axis.


.. py:method:: TrackParticles.iterParticles(timestep, chunksize=1)

  This method, specific to the tracked particles, provides a fast iterator on chunks of particles
  for a given timestep. The argument ``chunksize`` is the number of particles in each chunk.
  Note that the data is *not ordered* by particle ID, meaning that particles are not ordered
  the same way from one timestep to another.

  The returned quantity for each iteration is a python dictionary containing key/value
  pairs ``axis:array``, where ``axis`` is the name of the particle characteristic (``"x"``,
  ``"px"``, etc.) and ``array`` contains the corresponding particle values.

  **Example**::

      S = happi.Open("path/to/my/results")        # Open the simulation
      Diag = S.TrackParticles("my_particles") # Open the tracked particles
      npart = 0
      sum_px = 0.
      # Loop particles of timestep 100 by chunks of 10000
      for particle_chunk in Diag.iterParticles(100, chunksize=10000):
          npart  += particle_chunk["px"].size
          sum_px += particle_chunk["px"].sum()
      # Calculate the average px
      mean_px = sum_px / npart

.. py:method:: Field.getXmoved( timestep )

  Specific to Field diagnostics, this method returns the displacement of the moving
  window at the required ``timestep``.

----

Export 2D or 3D data to VTK
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Field.toVTK( numberOfPieces=1 )
               Probe.toVTK( numberOfPieces=1 )
               ParticleBinning.toVTK( numberOfPieces=1 )
               Screen.toVTK( numberOfPieces=1 )
               TrackParticles.toVTK( rendering="trajectory", data_format="xml" )

  Converts the data from a diagnostic object to the vtk format.

  * ``numberOfPieces``: the number of files into which the data will be split.

  * ``rendering``: the type of output in the case of :py:meth:`TrackParticles`:

    * ``"trajectory"``: show particle trajectories. One file is generated for all trajectories.
    * ``"cloud"``: show a cloud of particles. One file is generated for each iteration.

  * ``data_format``: the data formatting in the case of :py:meth:`TrackParticles`,
    either ``"vtk"`` or ``"xml"``. The format ``"vtk"`` results in ascii.

  **Example for tracked particles**::

    S = happi.Open("path/to/my/results")
    tracked_particles = S.TrackParticles("electron", axes=["x","y","z","px","py","pz","Id"], timesteps=[1,10])
    # Create cloud of particles in separate files for each iteration
    tracked_particles.toVTK(rendering="cloud",data_format="xml");
    # Create trajectory in a single file
    tracked_particles.toVTK(rendering="trajectory",data_format="xml");

----

Plot the data at one timestep
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is the first method to plot the data. It produces a static image of the data
at one given timestep.

.. py:method:: Scalar.plot(...)
               Field.plot(...)
               Probe.plot(...)
               ParticleBinning.plot(...)
               TrackParticles.plot(...)
               Screen.plot(...)

  All these methods have the same arguments described below.

.. py:function:: plot(timestep=None, saveAs=None, axes=None, **kwargs)

  | If the data is 1D, it is plotted as a **curve**.
  | If the data is 2D, it is plotted as a **map**.
  | If the data is 0D, it is plotted as a **curve** as function of time.

  * ``timestep``: The iteration number at which to plot the data.
  * ``saveAs``: name of a directory where to save each frame as figures.
    You can even specify a filename such as ``mydir/prefix.png`` and it will automatically
    make successive files showing the timestep: ``mydir/prefix0.png``, ``mydir/prefix1.png``,
    etc.
  * ``axes``: Matplotlib's axes handle on which to plot. If None, make new axes.

  Other keyword-arguments (``kwargs``) include:

  * ``figure``: The figure number that is passed to matplotlib.
  * ``vmin``, ``vmax``: data value limits.
  * ``xmin``, ``xmax``, ``ymin``, ``ymax``: axes limits.
  * ``xfactor``, ``yfactor``: factors to rescale axes.
  * ``side``: ``"left"`` (by default) or ``"right"`` puts the y-axis on the left- or the right-hand-side.
  * ``transparent``: ``None`` (by default), ``"over"``, ``"under"`` or ``"both"`` makes the colormap transparent outside the requested boundary.
  * Many Matplotlib arguments listed in :ref:`advancedOptions`.

**Example**::

    S = happi.Open("path/to/my/results")
    S.ParticleBinning(1).plot(timestep=40, vmin=0, vmax=1e14)

----

Plot the data streaked over time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This second type of plot works only for 1D data. All available timesteps
are streaked to produce a 2D image where the second axis is time.

.. py:method:: Scalar.streak(...)
               Field.streak(...)
               Probe.streak(...)
               ParticleBinning.streak(...)
               TrackParticles.streak(...)
               Screen.streak(...)

  All these methods have the same arguments described below.

.. py:function:: streak(saveAs=None, axes=None, **kwargs)

  All arguments are identical to those of ``plot``, with the exception of ``timestep``.

**Example**::

    S = happi.Open("path/to/my/results")
    S.ParticleBinning(1).streak()

----

Animated plot
^^^^^^^^^^^^^

This third plotting method animates the data over time.

.. py:method:: Scalar.animate(...)
               Field.animate(...)
               Probe.animate(...)
               ParticleBinning.animate(...)
               TrackParticles.animate(...)
               Screen.animate(...)

  All these methods have the same arguments described below.

.. py:function:: animate(movie="", fps=15, dpi=200, saveAs=None, axes=None)

  All arguments are identical to those of ``streak``, with the addition of:

  * ``movie``: name of a file to create a movie, such as ``"movie.avi"`` or  ``"movie.gif"``.
    If ``movie=""`` no movie is created.
  * ``fps``: number of frames per second (only if movie requested).
  * ``dpi``: number of dots per inch (only if movie requested).

**Example**::

    S = happi.Open("path/to/my/results")
    S.ParticleBinning(1).animate()

----

Simultaneous plotting of multiple diagnostics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:function:: happi.multiPlot(diag1, diag2, ... , **kwargs)

  Makes an animated figure containing several plots (one for each diagnostic).
  If all diagnostics are of similar type, they may be overlayed on only one plot.

  * ``diag1``, ``diag2``, etc.
     | Diagnostics prepared by ``Scalar()``, ``Field()``, ``Probe()``, etc.

  Keyword-arguments ``kwargs`` are:

  * ``figure``: The figure number that is passed to matplotlib (default is 1).
  * ``shape``: The arrangement of plots inside the figure. For instance, ``[2, 1]``
    makes two plots stacked vertically, and ``[1, 2]`` makes two plots stacked horizontally.
    If absent, stacks plots vertically.
  * ``movie`` : filename to create a movie.
  * ``fps`` : frames per second for the movie.
  * ``dpi`` : resolution of the movie.
  * ``saveAs``: name of a directory where to save each frame as figures.
    You can even specify a filename such as ``mydir/prefix.png`` and it will automatically
    make successive files showing the timestep: ``mydir/prefix0.png``, ``mydir/prefix1.png``, etc.
  * ``skipAnimation`` : if True, plots only the last frame.
  * ``timesteps``: same as the ``timesteps`` argument of the :py:func:`plot` method.

**Example**::

    S = happi.Open("path/to/my/results")
    A = S.Probe(probeNumber=0, field="Ex")
    B = S.ParticleBinning(diagNumber=1)
    happi.multiPlot( A, B, figure=1 )

..

  This plots a Probe and a ParticleBinning on the same figure, and makes an animation for all available timesteps.


----

.. _advancedOptions:

Advanced plotting options
^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to ``figure``, ``vmin``, ``vmax``, ``xmin``, ``xmax``, ``ymin`` and ``ymax``,
there are many more optional arguments. They are directly passed to the *matplotlib* package.

Options for the figure: ``figsize``, ``dpi``, ``facecolor``, ``edgecolor``

    Please refer to `matplotlib's figure options <http://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.figure>`_.

Options for the axes frame: ``aspect``, ``axis_bgcolor``, ``frame_on``, ``position``, ``title``, ``visible``,
``xlabel``, ``xscale``, ``xticklabels``, ``xticks``, ``ylabel``, ``yscale``, ``yticklabels``, ``yticks``, ``zorder``

    Please refer to matplotlib's axes options: the same as functions starting with ``set_`` listed `here <http://matplotlib.org/api/axes_api.html>`_.

Options for the lines: ``color``, ``dashes``, ``drawstyle``, ``fillstyle``, ``label``, ``linestyle``, ``linewidth``,
``marker``, ``markeredgecolor``, ``markeredgewidth``, ``markerfacecolor``, ``markerfacecoloralt``,
``markersize``, ``markevery``, ``visible``, ``zorder``

    Please refer to `matplotlib's line options <http://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.plot>`_.

Options for the image: ``cmap``, ``aspect``, ``interpolation``

    Please refer to `matplotlib's image options <http://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.imshow>`_.

Options for the colorbar: ``cbaspect``, ``orientation``, ``fraction``, ``pad``, ``shrink``, ``anchor``, ``panchor``,
``extend``, ``extendfrac``, ``extendrect``, ``spacing``, ``ticks``, ``format``, ``drawedges``

    Please refer to `matplotlib's colorbar options <http://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.colorbar>`_.

Options for the tick labels: ``style_x``, ``scilimits_x``, ``useOffset_x``, ``style_y``, ``scilimits_y``, ``useOffset_y``

    Please refer to `matplotlib's tick label format <http://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.ticklabel_format.html>`_.


**Example**:

  To choose a gray colormap of the image, use ``cmap="gray"``::

    S = happi.Open("path/to/my/results")
    S.ParticleBinning(0, figure=1, cmap="gray") .plot()

..

  Many colormaps are available from the *matplotlib* package. With ``cmap=""``, you will get a list of available colormaps.
  Smilei's default colormaps are: ``smilei``, ``smilei_r``, ``smileiD`` and ``smileiD_r``.

----

Update the plotting options
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: Scalar.set(...)
               Field.set(...)
               Probe.set(...)
               ParticleBinning.set(...)
               Screen.set(...)


  **Example**::

    S = happi.Open("path/to/my/results")
    A = S.ParticleBinning(diagNumber=0, figure=1, vmax=1)
    A.plot( figure=2 )
    A.set( vmax=2 )
    A.plot()

----

Other tools in ``happi``
^^^^^^^^^^^^^^^^^^^^^^^^

.. py:method:: happi.openNamelist(namelist)

  Reads a namelist and stores all its content in the returned object.

  * ``namelist``: the path to the namelist.

**Example**::

  namelist = happi.openNamelist("path/no/my/namelist.py")
  print namelist.Main.timestep


----

Tutorials
^^^^^^^^^

A dedicated page for learning how to run and analyse Smilei simulations
is `available here <https://smileipic.github.io/tutorials>`_.
