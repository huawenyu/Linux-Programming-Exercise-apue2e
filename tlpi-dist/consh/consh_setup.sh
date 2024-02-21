#!/bin/sh
#
# consh_setup.sh
#
# (C) 2023, Michael Kerrisk
#
# Licensed under the GNU General Public License version 2 or later

usage()
{
cat << EOF 1>&2

Usage: $0 [-c <cgroup>] [-h <hostname>] <lower-dir> <overlay-dir>

<lower-dir> is the pathname of an existing directory that will be used as the
        lower layer in the OverlayFS union mount for the "container".
<overlay-dir> is the pathname that will be used create a new directory where
	the other pieces (upper, work, merged) of the OverlayFS union mount
	for the "container" will be set up. The overlay mount will be created
	at '<overlay-dir>/merged'
-c <cgroup> can be used specify the pathname of a cgroup into which the
	"container" should be placed. The pathname is interpreted relative to
	the cgroup root directory, and the cgroup is created if it does not
	already exist.
-h <hostname> can be used to specify the hostname that will be used in the
        "container".
EOF
    exit 1
}

# Command-line option parsing

cgroup=""
host="consh-host"
export VERBOSE=""
while getopts ":c:h:v" optname "$@"; do
    case "$optname" in
    c)  cgroup=$OPTARG
        ;;
    h)  host=$OPTARG
        ;;
    v)  VERBOSE="x"
        ;;
    *)  echo "Unknown option: $OPTARG" 1>&2
	usage
        ;;
    esac
done

shift $(expr $OPTIND - 1)

if test $# -lt 2; then
    echo "Missing arguments" 1>&2
    usage
fi

lower=$1
ovly_dir=$2
shift 2

if ! test -e $lower; then
    echo "Error: '$lower' does not exist!" 1>&2
    exit 1
fi

# In the following steps, we create the mount that will be used as the root
# filesystem in the container. The mount is a union mount built by using
# OverlayFS to combine a set of directories created under <fs-dir>.

if test -n "$VERBOSE"; then
    echo "Creating overlay dirs in '$ovly_dir'"
fi
mkdir -p $ovly_dir/upper $ovly_dir/work
	# Avoid "mkdir -p $ovly_dir/{upper,work}"
	# because not supported by busybox shell

mkdir $ovly_dir/merged		# Create mount point

if test -n "$VERBOSE"; then
    echo "Creating overlay mount at '$ovly_dir/merged'"
fi
sudo mount -t overlay \
		-o lowerdir=$lower \
		-o upperdir=$ovly_dir/upper \
		-o  workdir=$ovly_dir/work \
		    overlay $ovly_dir/merged

cd $ovly_dir

# As a demonstration, create a file that is private to this container.

manifest=merged/MANIFEST
echo "Created at:  $(date)"   > $manifest
echo "Creator UID: $(id -u)" >> $manifest
echo "Creator PID: $$"       >> $manifest

# If a cgroup was specified, create that cgroup and move this shell into the
# cgroup.

cgflag=""
if test "X$cgroup" != "X"; then
    echo "Using cgroup: $cgroup" >> $manifest
    if test -n "$VERBOSE"; then
        echo "Creating cgroup: $cgroup"
    fi

    cgpath="/sys/fs/cgroup/$cgroup"
    sudo mkdir -p $cgpath

    sudo sh -c "echo $$ > $cgpath/cgroup.procs"

    # Delegation: change ownership of the cgroup directory and certain files
    # inside that directory.

    sudo sh -c 'cd '$cgpath'
		dlgt_files=$(ls $(cat /sys/kernel/cgroup/delegate) 2> /dev/null)
		chown '$(id -u):$(id -g)' . $dlgt_files'

    cgflag="--cgroup"
fi

# We use unshare(1) to launch a shell in new namespaces. Some points deserve
# explanation:
#
# - We use busybox to launch the shell. This ensures that (a) the shell is
#   statically linked (and thus has no external shared library dependencies),
#   and (b) we are running the same shall as would be run by using 'sh' inside
#   the "container".
#
# - We set the ENV environment to point to a script that will be executed as
#   a start-up script by the new shell. This script performs the set-up steps
#   that are required after the namespaces have been created.
#
# - We use the --fork option to ensure that a child process is created.
#   That child will have PID in the new PID namespace.
#
# - One blemish in this set-up is that the "unshare" parent process will
#   also be in the cgroup. I can see no tidy way to avoid this (at least
#   not if we want to run an interactive shell in the container and we use the
#   standard unshare(1) command to do the set-up). Consequently, the only way
#   to remove the parent process from the cgroup would be via a manual step
#   in a separate terminal window.
#
# - We use the shell 'exec' built-in, so that the shell executing this script
#   is replaced by the "unshare" executable. This allows us to avoid the
#   creation of yet another process that would be a member of the cgroup.
#
if test -n "$VERBOSE"; then
    echo "Using 'unshare' to launch a shell in new namespaces"
fi

exec env -i HOME="/root" PATH="/usr/sbin:/usr/bin:/sbin:/bin" \
	    HOSTNAME="$host" \
	    VERBOSE="$VERBOSE" \
	    ENV=$(dirname $0)/consh_post_setup.sh \
	unshare --user --map-root-user --pid --fork \
	        --mount --net --ipc --uts --cgroup \
            busybox sh
