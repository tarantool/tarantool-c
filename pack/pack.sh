# Travis-CI wrapper for parallel builds
if [ $# -eq 3 ] ; then
    make build-$1 os=$2 dist=$3
    make export-$1 os=$2 dist=$3
elif [ $# -eq 5 ] ; then
    make os=$2 dist=$3 product=$4 uri=$5 build-$1
    make os=$2 dist=$3 export-$1
else
    echo 'build skipped'
    echo 'Usage:'
    echo './pack.sh [rpm/deb] <os> <distr>'
    echo 'Example: ./pack.sh rpm fedora 20'
    exit 0
fi
