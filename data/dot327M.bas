# .bash_profile modified for use with IPF-3.27.17 [26-March-2001 update]
# "root" user MUST also replace the default .bash_profile initialization file with this new version.
# File:  /root/.bash_profile    Syntax:  "cp ./sample.bash_profile ./bash_profile"

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
	. ~/.bashrc
fi

# User specific environment and startup programs

PATH=$PATH:$HOME/bin
ENV=$HOME/.bashrc
USERNAME=""

export USERNAME ENV PATH

mesg n

UIDPATH="/shr/ipf-3.27/exe/%U:/usr/lib/X11/uid/%U"
# path to locate help and sample data files
IPFDIRS="/shr/ipf-3.27/dat/"  
IPFROOTDIR="/shr/ipf-3.27"       
# optional alternative IPFDIRS=<IPF base case directory>
IPFSRV="/shr/ipf-3.27/exe/ipfsrv"
IPFSRV_CF="/shr/ipf-3.27/exe/ipfsrv" 
# IPF_SOCK_PATH=<IPF temporary directory to hold socket locking files>
IPF_SOCK_PATH="/shr/ipf-3.27/temp"
RUN_IPFSRV="/shr/ipf-3.27/exe/ipfsrv"

export UIDPATH IPFDIRS RUN_IPFSRV IPFSRV_CF IPF_SOCK_PATH IPFROOTDIR

/usr/games/fortune

# startx
