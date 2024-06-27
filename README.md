# NESO-hw-impurity-transport
A NESO-based solver for modelling impurity transport in a plasma governed by the Hasegawa-Wakatani equations.

## Building
The easiest way to build the app is to use the [Spack package manager](https://spack.readthedocs.io/en/latest/index.html).
The build process has been tested with **Spack v0.19.0**; later versions may also work, but aren't recommended.

1. Install Spack v0.19.0, via the [official instructions](https://spack.readthedocs.io/en/latest/getting_started.html#installation).
On Debian-based systems (e.g. Ubuntu) the following should work:

```bash
# Ensure all prerequisites are installed
apt update
apt install -y build-essential ca-certificates coreutils curl environment-modules gfortran git gpg lsb-release python3 python3-distutils python3-venv unzip zip

git clone -c feature.manyFiles=true -b v0.19.0 https://github.com/spack/spack.git $HOME/.spack

# Modify .bashrc such that spack is always initialised in new shells
echo 'export SPACK_ROOT=$HOME/.spack' >> $HOME/.bashrc
echo 'source $SPACK_ROOT/share/spack/setup-env.sh' >> $HOME/.bashrc
export SPACK_ROOT=$HOME/.spack
source $SPACK_ROOT/share/spack/setup-env.sh
```

2. Next, having git-cloned this repository, change to the root directory (containing this readme) and obtain the required versions of NESO and its dependencies with:
```bash
git submodule update --init --recursive
```

3. The app can then be built with:
```bash
spack env activate ./spack -p
spack install -j [np]
```

where \[np\] is the number of parallel processes to use in the build.
Note that some of the dependencies (particularly nektar++) can take some time to install.

The solver executable will be generated in `spack-build-<hash>`, where `<hash>`` is an 8 character hash generated by Spack.`
To rebuild the app after modifying the source code, repeat the `spack install` with the spack environment active.

## Running examples
Configuration files for 2D and 3D examples (with and without impurity tracer particles) can be found in the `./examples` directory.
An easy way to run the examples is (from the top level of the repository):
```bash
# Load the mpich spack module
spack load mpich
# Set up and run an example via the helper script
./scripts/run_eg.sh [example_name]
```

- This will copy `./examples/[example_name]` to `./runs/[example_name]` and run the solver in that directory with 4 MPI processes by default.
- To run with a different number of MPI processes, use `<-n num_MPI> `
- The solver executable is assumed to be in the most recently modified `spack-build*` directory. To choose a different build location, use `<-b build_dir_path>`.

To change the number of openMP threads used by each process, use
```bash
# Run an example with OMP and MPI
OMP_NUM_THREADS=[nthreads] ./scripts/run_eg.sh [example_name]
```

## Postprocessing
- Particle output is generated in an hdf5 file. See [particle_velhist.py](./postproc/particle_velhist.py) for an example of reading and postprocessing (requires [these packages](./postproc/requirements.txt)).
- Fluid output is generated as nektar++ checkpoint files. One way to visualise is them is to convert to .vtu using Nektar's `FieldConvert` tool and then use Paraview:
```bash
# Convert to vtu using FieldConvert (requires xml files as args)
cd runs/[example_name]
spack load nektar
FieldConvert-rg [config_xml] [mesh_xml] [chk_name] [vtu_name]
# e.g. FieldConvert-rg hw.xml square.xml hw_100.chk hw_100.vtu
``` 

## Known issues / annoyances

- Particle hdf5 output isn't finalised until the solver finishes, so interrupted runs will generate an unreadable file.
- If all fluid boundary conditions are periodic, the nektar++ routine used to solve for the potential arbitrarily sets one of the degrees of freedom to zero in order to avoid a singular solution.  This leads to an artifact in the potential (which eventually creates similar artifacts in the vorticity and density fields, see below).

<img src="./docs/img/phi_artifact.png" center="left" width="400">

The nektar++ developers are currently working on a fix for this.
---