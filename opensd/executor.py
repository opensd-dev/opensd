from numbers import Integral
import subprocess

def _process_CLI_arguments(opensd_exec='opensd', mpi_args=None, petsc_args = None, threads = None):
    """Converts user-readable flags in to command-line arguments to be run with
    the OpenSD executable via subprocess.

    Parameters
    ----------
    opensd_exec : str, optional
        Path to OpenMC executable. Defaults to 'opensd'.
    mpi_args : list of str, optional
        MPI execute command and any additional MPI arguments to pass,
        e.g., ['mpiexec', '-n', '8'].
    threads : int, optional
        Number of OpenMP threads. If OpenSD is compiled with OpenMP threading
        enabled, the default is implementation-dependent but is usually equal
        to the number of hardware threads available (or a value set by the
        :envvar:`OMP_NUM_THREADS` environment variable).

    Returns
    -------
    args : Iterable of str
        The runtime flags converted to CLI arguments of the OpenSD executable

    """

    args = [opensd_exec]

    if mpi_args is not None:
        args = mpi_args + args

    if petsc_args is not None:
        args += petsc_args

    if isinstance(threads, Integral) and threads > 0:
        args += ['-s', str(threads)]

    return args

def _run(args, output, cwd):
    # Launch a subprocess
    p = subprocess.Popen(args, cwd=cwd, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, universal_newlines=True)

    # Capture and re-print OpenSD output in real-time
    lines = []
    while True:
        # If OpenSD is finished, break loop
        line = p.stdout.readline()
        if not line and p.poll() is not None:
            break

        lines.append(line)
        if output:
            # If user requested output, print to screen
            print(line, end='')

    # # Raise an exception if return status is non-zero
    # if p.returncode != 0:
        # # Get error message from output and simplify whitespace
        # output = ''.join(lines)
        # if 'ERROR: ' in output:
            # _, _, error_msg = output.partition('ERROR: ')
        # elif 'what()' in output:
            # _, _, error_msg = output.partition('what(): ')
        # else:
            # error_msg = 'OpenSD aborted unexpectedly.'
        # error_msg = ' '.join(error_msg.split())

        # raise RuntimeError(error_msg)

def run(output=True, cwd='.', opensd_exec='opensd', mpi_args=None, petsc_args=None, threads=None):
    """Run an OpenSD simulation.

    Parameters
    ----------
    output : bool
        Capture OpenMC output from standard out
    cwd : str, optional
        Path to working directory to run in. Defaults to the current working
        directory.
    opensd_exec : str, optional
        Path to OpenSD executable. Defaults to 'opensd'.
    mpi_args : list of str, optional
        MPI execute command and any additional MPI arguments to pass, e.g.,
        ['mpiexec', '-n', '8'].
    threads : int, optional
        Number of OpenMP threads. If OpenSD is compiled with OpenMP threading
        enabled, the default is implementation-dependent but is usually equal to
        the number of hardware threads available (or a value set by the
        :envvar:`OMP_NUM_THREADS` environment variable).

    Raises
    ------
    RuntimeError
        If the `opensd` executable returns a non-zero status

    """
    args = _process_CLI_arguments(opensd_exec=opensd_exec, mpi_args=mpi_args, petsc_args=petsc_args, threads=threads)

    _run(args, output, cwd)
