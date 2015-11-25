# Travis-CI wrapper for parallel builds
if [ $# -eq 5 ] ; then
    make build-$1 os=$2 dist=$3 branch=$4
    make export-$1 os=$2 dist=$3 repo=$5
elif [ $# -eq 5 ] ; then
    make os=$2 dist=$3 branch=$4 product=$6 uri=$7 build-$1
    make os=$2 dist=$3 repo=$5 export-$1
else
    echo 'build skipped'
    echo 'Usage:'
    echo './pack.sh [rpm/deb] <os> <distr>'
    echo 'Example: ./pack.sh rpm fedora 20'
    exit 0
fi
