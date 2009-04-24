#!/bin/sh
# even though -n is compliment of -z it doesn't work as expected hence the !
usage ()
{
    echo -e "\n" \
            "Purpose: run unit tests unless told not to.\n" \
            "Usage: [--unit-test-path <path to unit tests>]\n" \
	    "       [--excludes-file <path to excludes file>]\n" \
	    "       [--help|-h|-?]\n" \
            "\n" \
            "*** Following is not yet implemented ***\n" \
            "Script will exclude tests if 'tst_excludes' file\n" \
            "is present and the test's name has been added to\n" \
            "exclude_tests='' in the file.  List is space seperated.\n" \
            "Create excludes file if not present.\n"
}

while [ $# -ne 0 ]; do
    case $1 in
        --unit-tests-path)
            if [ $# -ge 2 ]; then
                test_dir_path="$2"
                shift 1
            fi
            ;;
        --excludes-file)
            if [ $# -ge 2 ]; then
                test_excludes_path="$2"
                shift 1
            fi
            ;;
       --help|-h|-?)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

# set -x
# the following handles different paths being set or not and tests
current_dir=`pwd`
# if not set then set to current working dir path
test_dir_path=${test_dir_path-$current_dir/'tests'}
# if we can't see the tests dir stop
[ ! -d $test_dir_path ] && echo "Unit tests path: $test_dir_path does not exist" && exit 1
# if not set or set but empty assume relative to test_dir_path
test_excludes_path=${test_excludes_path:-$test_dir_path/tst_excludes}

# see if we have an tst_excludes_file
[ -f $test_excludes_path ] && . $test_excludes_path || exclude_tests=""
# run through executing tests
for each_test in `find $test_dir_path -type f -name "tst_*"`; do
    # comparison to go here
#    if [ ${#exclude_test[@]} -ne 0 ]; then
#        test_excluded=0
#    else
#        for (( i = 0 ; i < ${#exclude_tests[@]} ; i++ ))
#        do
#            if [ ${exclude_tests[i]} == ${test_name} ]; then
#                test_exlcuded=1
#                break
#            fi
#            echo $test_name
#        done
#    fi
#done
#    if [ ${test_excluded} -eq 0 ]; then
        test_exec=$each_test
        mkdir -p /tmp/libqmf-tests
        [ -x $test_exec ] && $test_exec > /tmp/libqmf-tests/$test_exec.log
#    fi
done
exit 0
