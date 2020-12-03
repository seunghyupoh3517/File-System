#!/bin/bash

set -o pipefail
#set -xv # debug

#
# Logging helpers
#
log() {
    echo -e "${*}"
}

info() {
    log "Info: ${*}"
}
warning() {
    log "Warning: ${*}"
}
error() {
    log "Error: ${*}"
}
die() {
    error "${*}"
    exit 1
}

#
# Scoring helpers
#
select_line() {
	# 1: string
	# 2: line to select
	echo $(echo "${1}" | sed "${2}q;d")
}

fail() {
	# 1: got
	# 2: expected
    log "Fail: got '${1}' but expected '${2}'"
}

pass() {
	# got
    log "Pass: ${1}"
}

compare_lines() {
	# 1: output
	# 2: expected
    # 3: score (output)
	declare -a output_lines=("${!1}")
	declare -a expect_lines=("${!2}")
    local __score=$3
    local partial="0"

    # Amount of partial credit for each correct output line
    local step=$(bc -l <<< "1.0 / ${#expect_lines[@]}")

    # Compare lines, two by two
	for i in ${!output_lines[*]}; do
		if [[ "${output_lines[${i}]}" =~ "${expect_lines[${i}]}" ]]; then
			pass "${output_lines[${i}]}"
            partial=$(bc <<< "${partial} + ${step}")
		else
			fail "${output_lines[${i}]}" "${expect_lines[${i}]}" ]]
		fi
	done

    # Return final score
    eval ${__score}="'${partial}'"
}

#
# Generic function for running FS tests
#
run_test() {
    # These are global variables after the test has run so clear them out now
	unset STDOUT STDERR RET

    # Create temp files for getting stdout and stderr
    local outfile=$(mktemp)
    local errfile=$(mktemp)

    timeout 2 "${@}" >${outfile} 2>${errfile}

    # Get the return status, stdout and stderr of the test case
    RET="${?}"
    STDOUT=$(cat "${outfile}")
    STDERR=$(cat "${errfile}")

    # Deal with the possible timeout errors
    [[ ${RET} -eq 127 ]] && warning "Something is wrong (the executable probably doesn't exists)"
    [[ ${RET} -eq 124 ]] && warning "Command timed out..."

    # Clean up temp files
    rm -f "${outfile}"
    rm -f "${errfile}"
}

#
# Generic function for capturing output of non-test programs
#
run_tool() {
    # Create temp files for getting stdout and stderr
    local outfile=$(mktemp)
    local errfile=$(mktemp)

    timeout 2 "${@}" >${outfile} 2>${errfile}

    # Get the return status, stdout and stderr of the test case
    local ret="${?}"
    local stdout=$(cat "${outfile}")
    local stderr=$(cat "${errfile}")

    # Log the output
    [[ ! -z ${stdout} ]] && info "${stdout}"
    [[ ! -z ${stderr} ]] && info "${stderr}"

    # Deal with the possible timeout errors
    [[ ${ret} -eq 127 ]] && warning "Tool execution failed..."
    [[ ${ret} -eq 124 ]] && warning "Tool execution timed out..."

    # Clean up temp files
    rm -f "${outfile}"
    rm -f "${errfile}"
}

#
# Phase 1
#

# Info on empty disk
run_fs_info() {
    log "\n--- Running ${FUNCNAME} ---"

    run_tool ./fs_make.x test.fs 100
    run_test ./test_fs.x info test.fs
    rm -f test.fs

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "1")")
    line_array+=("$(select_line "${STDOUT}" "2")")
    line_array+=("$(select_line "${STDOUT}" "3")")
    line_array+=("$(select_line "${STDOUT}" "4")")
    line_array+=("$(select_line "${STDOUT}" "5")")
    line_array+=("$(select_line "${STDOUT}" "6")")
    line_array+=("$(select_line "${STDOUT}" "7")")
    line_array+=("$(select_line "${STDOUT}" "8")")
    local corr_array=()
    corr_array+=("FS Info:")
    corr_array+=("total_blk_count=103")
    corr_array+=("fat_blk_count=1")
    corr_array+=("rdir_blk=2")
    corr_array+=("data_blk=3")
    corr_array+=("data_blk_count=100")
    corr_array+=("fat_free_ratio=99/100")
    corr_array+=("rdir_free_ratio=128/128")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "Score: ${score}"
}

# Info with files
run_fs_info_full() {
    log "\n--- Running ${FUNCNAME} ---"

	run_tool ./fs_make.x test.fs 100
	run_tool dd if=/dev/urandom of=test-file-1 bs=2048 count=1
	run_tool dd if=/dev/urandom of=test-file-2 bs=2048 count=2
	run_tool dd if=/dev/urandom of=test-file-3 bs=2048 count=4
	run_tool ./fs_ref.x add test.fs test-file-1
	run_tool ./fs_ref.x add test.fs test-file-2
	run_tool ./fs_ref.x add test.fs test-file-3

	run_test ./test_fs.x info test.fs
	rm -f test-file-1 test-file-2 test-file-3 test.fs

	local line_array=()
	line_array+=("$(select_line "${STDOUT}" "7")")
	line_array+=("$(select_line "${STDOUT}" "8")")
	local corr_array=()
	corr_array+=("fat_free_ratio=95/100")
	corr_array+=("rdir_free_ratio=125/128")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "Score: ${score}"
}

#
# Phase 2
#

# make fs with fs_make.x, add with test_fs.x, ls with fs_ref.x
run_fs_simple_create() {
    log "\n--- Running ${FUNCNAME} ---"

	run_tool ./fs_make.x test.fs 10
	run_tool dd if=/dev/zero of=test-file-1 bs=10 count=1
	run_tool timeout 2 ./test_fs.x add test.fs test-file-1
	run_test ./fs_ref.x ls test.fs
	rm -f test.fs test-file-1

	local line_array=()
	line_array+=("$(select_line "${STDOUT}" "2")")
	local corr_array=()
	corr_array+=("file: test-file-1, size: 10, data_blk: 1")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "Score: ${score}"
}

# make fs with fs_make.x, add two with test_fs.x, ls with fs_ref.x
run_fs_create_multiple() {
    log "\n--- Running ${FUNCNAME} ---"

	run_tool ./fs_make.x test.fs 10
	run_tool dd if=/dev/zero of=test-file-1 bs=10 count=1
	run_tool dd if=/dev/zero of=test-file-2 bs=10 count=1
	run_tool timeout 2 ./test_fs.x add test.fs test-file-1
	run_tool timeout 2 ./test_fs.x add test.fs test-file-2

	run_test ./fs_ref.x ls test.fs

	rm -f test.fs test-file-1 test-file-2

	local line_array=()
	line_array+=("$(select_line "${STDOUT}" "2")")
	line_array+=("$(select_line "${STDOUT}" "3")")
	local corr_array=()
	corr_array+=("file: test-file-1, size: 10, data_blk: 1")
	corr_array+=("file: test-file-2, size: 10, data_blk: 2")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "Score: ${score}"
}

run_comprehensive(){
    echo -e "\n\n";
    echo "Creating Virtual Disk of size 8192";
    rm *driver;
    ./fs_make.x our_driver.fs 8192;
    ./fs_make.x ref_driver.fs 8192;


    echo -e "\n\n";
    echo "Testing info";
    ./test_fs.x info our_driver.fs > our_info.txt;
    ./fs_ref.x info ref_driver.fs > ref_info.txt;
    diff our_info.txt ref_info.txt;
    rm our_info.txt ref_info.txt;


    echo -e "\n\n";
    echo "Testing empty ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing small file addition";
    ./test_fs.x add our_driver.fs file1.txt > our_add.txt;
    ./fs_ref.x add ref_driver.fs file1.txt > ref_add.txt;
    diff our_add.txt ref_add.txt;
    rm ref_add.txt our_add.txt;


    echo -e "\n\n";
    echo "Testing small file read";
    ./test_fs.x cat our_driver.fs file1.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs file1.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing large file addition";
    ./test_fs.x add our_driver.fs file2.txt > our_add.txt;
    ./fs_ref.x add ref_driver.fs file2.txt > ref_add.txt;
    diff our_add.txt ref_add.txt;
    rm ref_add.txt our_add.txt;


    echo -e "\n\n";
    echo "Testing large file read";
    ./test_fs.x cat our_driver.fs file2.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs file2.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing invalid file read";
    ./test_fs.x cat our_driver.fs blank.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs blank.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing 2 file ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing 1 file removal";
    ./test_fs.x rm our_driver.fs file1.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file1.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing 1 file ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing info";
    ./test_fs.x info our_driver.fs > our_info.txt;
    ./fs_ref.x info ref_driver.fs > ref_info.txt;
    diff our_info.txt ref_info.txt;
    rm our_info.txt ref_info.txt;


    echo -e "\n\n";
    echo "Testing stat of existing file";
    ./test_fs.x stat our_driver.fs file2.txt > our_stat.txt;
    ./fs_ref.x stat ref_driver.fs file2.txt > ref_stat.txt;
    diff our_stat.txt ref_stat.txt;
    rm our_stat.txt ref_stat.txt;


    echo -e "\n\n";
    echo "Testing stat of nonexisting file";
    ./test_fs.x stat our_driver.fs file1.txt > our_stat.txt;
    ./fs_ref.x stat ref_driver.fs file1.txt > ref_stat.txt;
    diff our_stat.txt ref_stat.txt;
    rm our_stat.txt ref_stat.txt;


    echo -e "\n\n";
    echo "Testing final file removal";
    ./test_fs.x rm our_driver.fs file2.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file2.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing invalid file removal";
    ./test_fs.x rm our_driver.fs file2.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file2.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing empty ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;

    echo -e "\n\n";
    echo "Testing Completed to Driver size 8192";

    echo -e "\n\n";
    echo "Testing script script.example";
    ./test_fs.x script our_driver.fs scripts/script.example > our_script.txt;
    ./fs_ref.x script ref_driver.fs scripts/script.example > ref_script.txt;
    diff our_script.txt ref_script.txt;
    rm our_script.txt ref_script.txt;


    echo -e "\n\n";
    echo "Testing script mis_read_index_overwrite";
    ./test_fs.x script our_driver.fs scripts/mis_read_index_overwrite > our_script.txt;
    ./fs_ref.x script ref_driver.fs scripts/mis_read_index_overwrite > ref_script.txt;
    diff our_script.txt ref_script.txt;
    rm our_script.txt ref_script.txt;

    echo -e "\n\n";
    echo "Creating Virtual Disk of size 4";
    rm our_driver.fs ref_driver.fs;
    ./fs_make.x our_driver.fs 4;
    ./fs_make.x ref_driver.fs 4;


    echo -e "\n\n";
    echo "Testing info";
    ./test_fs.x info our_driver.fs > our_info.txt;
    ./fs_ref.x info ref_driver.fs > ref_info.txt;
    diff our_info.txt ref_info.txt;
    rm our_info.txt ref_info.txt;


    echo -e "\n\n";
    echo "Testing empty ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing small file addition";
    ./test_fs.x add our_driver.fs file1.txt > our_add.txt;
    ./fs_ref.x add ref_driver.fs file1.txt > ref_add.txt;
    diff our_add.txt ref_add.txt;
    rm ref_add.txt our_add.txt;


    echo -e "\n\n";
    echo "Testing small file read";
    ./test_fs.x cat our_driver.fs file1.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs file1.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing large file addition";
    ./test_fs.x add our_driver.fs file2.txt > our_add.txt;
    ./fs_ref.x add ref_driver.fs file2.txt > ref_add.txt;
    diff our_add.txt ref_add.txt;
    rm ref_add.txt our_add.txt;


    echo -e "\n\n";
    echo "Testing large file read";
    ./test_fs.x cat our_driver.fs file2.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs file2.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing invalid file read";
    ./test_fs.x cat our_driver.fs blank.txt > our_read.txt;
    ./fs_ref.x cat ref_driver.fs blank.txt > ref_read.txt;
    diff our_read.txt ref_read.txt;
    rm our_read.txt ref_read.txt;


    echo -e "\n\n";
    echo "Testing 2 file ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing 1 file removal";
    ./test_fs.x rm our_driver.fs file1.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file1.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing 1 file ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing info";
    ./test_fs.x info our_driver.fs > our_info.txt;
    ./fs_ref.x info ref_driver.fs > ref_info.txt;
    diff our_info.txt ref_info.txt;
    rm our_info.txt ref_info.txt;


    echo -e "\n\n";
    echo "Testing stat of existing file";
    ./test_fs.x stat our_driver.fs file2.txt > our_stat.txt;
    ./fs_ref.x stat ref_driver.fs file2.txt > ref_stat.txt;
    diff our_stat.txt ref_stat.txt;
    rm our_stat.txt ref_stat.txt;


    echo -e "\n\n";
    echo "Testing stat of nonexisting file";
    ./test_fs.x stat our_driver.fs file1.txt > our_stat.txt;
    ./fs_ref.x stat ref_driver.fs file1.txt > ref_stat.txt;
    diff our_stat.txt ref_stat.txt;
    rm our_stat.txt ref_stat.txt;


    echo -e "\n\n";
    echo "Testing final file removal";
    ./test_fs.x rm our_driver.fs file2.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file2.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing invalid file removal";
    ./test_fs.x rm our_driver.fs file2.txt > our_rm.txt;
    ./fs_ref.x rm ref_driver.fs file2.txt > ref_rm.txt;
    diff our_rm.txt ref_rm.txt;
    rm our_rm.txt ref_rm.txt;


    echo -e "\n\n";
    echo "Testing empty ls";
    ./test_fs.x ls our_driver.fs > our_ls.txt;
    ./fs_ref.x ls ref_driver.fs > ref_ls.txt;
    diff our_ls.txt ref_ls.txt;
    rm our_ls.txt ref_ls.txt;


    echo -e "\n\n";
    echo "Testing Completed to Driver size 4";
    rm our_driver.fs ref_driver.fs
}
#
# Run tests
#
run_tests() {
	# Phase 1
	run_fs_info
	run_fs_info_full
	# Phase 2
	run_fs_simple_create
	run_fs_create_multiple

    run_comprehensive
}

make_fs() {
    # Compile
    make > /dev/null 2>&1 ||
        die "Compilation failed"

    local execs=("test_fs.x" "fs_make.x" "fs_ref.x")

    # Make sure executables were properly created
    local x
    for x in "${execs[@]}"; do
        if [[ ! -x "${x}" ]]; then
            die "Can't find executable ${x}"
        fi
    done
}

make_fs
run_tests
