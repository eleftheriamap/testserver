################################################ imports #################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

import subprocess 
import os.path
import filecmp
import re
import json

################################################ globals #################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

PATH_TO_TESTS = './test/test_cases/'
PATH_TO_EXPECTED = './test/expected_results/'
PATH_TO_ACTUAL = './test/actual_results/'
TESTS = ['add',
         'adds',
         'and',
         'ands',
         'bcond',
         'bic',
         'bics',
         'br',
         'cmn',
         'cmp',
         'eon',
         'eor',
         'ldr',
         'madd',
         'mneg',
         'movk',
         'movn',
         'movz',
         'msub',
         'mul',
         'orn',
         'orr',
         'str',
         'sub',
         'subs',
         'tst']              
PATH_TO_SOLUTION = './solution/'


########################### functions to check if paths/directories exist ################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

'''
Function that returns the path of a filename
'''
def find_path(fname, path='.'):
    for root, dirs, files in os.walk(path, topdown=True):
        if fname in files:
            if fname == 'Makefile':
                return root
            return os.path.join(root, fname)
        
'''
Function that checks if requested path/file exist
Retruns an empty list if everything exists or the path names that are missing
This function is called in the beggining of each page in testserver.py
'''
def path_exists(paths):
    not_exists = []  

    #check if test_cases and solution directories exist
    for path in paths:
        if os.path.exists(path) == False:
            not_exists.append(path)
    
    return not_exists

'''
Function that checks if requested files exist
We can check if all test cases exist (fname=='all') or if a certain test case exists(fname=='test_case_name')
Retruns an empty list if everything exists or a list of files that are missing
This function is called in the beggining of each page in testserver.py
'''
def file_exists(fname='all', path = "", file_ending=""):
    not_exists = []  

    if fname == 'all':
        for test in TESTS:
            for i in range(0,20):
                #checks that file exists
                file_path = path + test + '/' + test + str(i) + file_ending
                #if not, add it to not_exists list
                if os.path.exists(file_path) == False:
                    not_exists.append(file_path)
   
    #check if specific file exists
    else:
        test = ''.join([i for i in fname if not i.isdigit()])
        file_path = path + test + '/' + fname + file_ending
        if os.path.exists(file_path) == False:
            not_exists.append(file_path)
            
    return not_exists

########################### functions run Makefile/compile ###############################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

'''
Function that generates tests
'''
def run_makefile(path_makefile):
    #compile
    try:
        command = ['make', '-C', path_makefile]
        subprocess.check_output(command, stderr=subprocess.STDOUT, text=True)
        return True
    
    #if an error is caught 
    except subprocess.CalledProcessError as e: 
        return e.stdout.splitlines()
    
##################################### execute assembler ###################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

'''
Function to execute assembler 
Takes as input the test case name (if fname==all, assembles all test cases)
We return the result of the test (CORRECT, INCORRECT,FAILED) the test cases names or error if the run_test has failed due to error
This function is called from testserver.py
'''
def assemble(fname, path_assembler):
    #create path to pu actual results
    if os.path.exists(PATH_TO_ACTUAL) == False:
        os.mkdir(PATH_TO_ACTUAL)
   
    #assemble all files
    if fname == 'all':
        results = []
        files = []
        #assemble all
        for test in TESTS:
            #create subdirectory for each test group
            if os.path.exists(PATH_TO_ACTUAL + test) == False:
                os.mkdir(PATH_TO_ACTUAL + test)
            for i in range(0,20):
                files.append(test+str(i))
                path_s = PATH_TO_TESTS + test + '/' + test + str(i) + '.s'
                path_actual_bin = PATH_TO_ACTUAL + test + '/' + test + str(i) + '.bin'
                path_expected_bin = PATH_TO_EXPECTED + test + '/' + test + str(i) + '_exp.bin'
                #execute assemble
                error = execute_assemble(fname, path_assembler, path_s, path_actual_bin)
                #if error append FAILED for this test
                if error:
                    results.append('FAILED')
                else:
                    if filecmp.cmp(path_actual_bin, path_expected_bin):
                        results.append('CORRECT')
                    else:
                        results.append('INCORRECT')
        return [files, results]
    
    #for a single file
    else:
        #get test group name
        test = ''.join([i for i in fname if not i.isdigit()])
        #create subdirectory for the test group
        if os.path.exists(PATH_TO_ACTUAL + test) == False:
            os.mkdir(PATH_TO_ACTUAL + test)
        path_s = PATH_TO_TESTS + test + '/' + fname + '.s'
        path_actual_bin = PATH_TO_ACTUAL + test + '/' + fname + '.bin'
        path_expected_bin = PATH_TO_EXPECTED + test + '/' + fname + '_exp.bin'
        #execute assemble
        error = execute_assemble(fname, path_assembler, path_s, path_actual_bin)
        #if error append FAILED for this test
        if error != False :
                results = 'FAILED'
                return [error, results, None, None]
        else:
            if filecmp.cmp(path_actual_bin, path_expected_bin):
                results = 'CORRECT'
            else:
                results = 'INCORRECT'
            expected_bin = read_binary_output(path_expected_bin)
            actual_bin = read_binary_output(path_actual_bin)
            return [error, results, expected_bin, actual_bin]

'''
Function to actually execute assembler 
Takes as input fname(all or path to specific test)
Executes run_tests.py which provides an out.json file with the results
We return True (run_tests.py executed without error) or the error (run_tests.py failed to execute due to error)
This function is called from assemble()
'''   
def execute_assemble(fname, path_assembler, path_s, path_actual_bin):
    #execute assemble
    try:
        command = [path_assembler, path_s, path_actual_bin]
        subprocess.check_output(command, stderr=subprocess.STDOUT, text=True)
        return False
    
    #if an error is caught 
    except subprocess.CalledProcessError as e: 
            if fname == 'all':
                return True
            else:
                return e

##################################### execute emualtor ###################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

'''
Function to execute emulator 
Takes as input the test case name (if fname==all, emulates all test cases)
We return the result of the test (CORRECT, INCORRECT,FAILED) the test cases names or error if the run_test has failed due to error
This function is called from testserver.py
'''
def emulate(fname, path_emulate):
    #create path to actual results
    if os.path.exists(PATH_TO_ACTUAL) == False:
        os.mkdir(PATH_TO_ACTUAL)
   
    #emulate all files
    if fname == 'all':
        results = []
        files = []
        #assemble all
        for test in TESTS:
            #create subdirectory for each test group
            if os.path.exists(PATH_TO_ACTUAL + test) == False:
                os.mkdir(PATH_TO_ACTUAL + test)
            for i in range(0,20):
                files.append(test+str(i))
                path_bin = PATH_TO_EXPECTED + test + '/' + test + str(i) + '_exp.bin'
                path_actual_out = PATH_TO_ACTUAL + test + '/' + test + str(i) + '.out'
                path_expected_out = PATH_TO_EXPECTED + test + '/' + test + str(i) + '_exp.out'
                #execute emulate
                if os.path.exists(path_expected_out) == True:
                    error = execute_emulate(fname, path_emulate, path_bin, path_actual_out)
                    #if error append FAILED for this test
                    if error:
                        results.append('FAILED')
                    else:
                        expected_out = read_output(path_expected_out)
                        actual_out = read_output(path_actual_out)
                        #remove spaces to compare outputs 
                        expected_out_no_spaces = remove_spaces(expected_out)
                        actual_out_no_spaces = remove_spaces(actual_out)
                        #compare outputs
                        if expected_out_no_spaces == actual_out_no_spaces:
                            results.append('CORRECT')
                        else:
                            results.append('INCORRECT')
        return [files, results]
    
    #for a single file
    else:
        #get test group name
        test = ''.join([i for i in fname if not i.isdigit()])
        #create subdirectory for the test group
        if os.path.exists(PATH_TO_ACTUAL + test) == False:
            os.mkdir(PATH_TO_ACTUAL + test)
        path_bin = PATH_TO_EXPECTED + test + '/' + fname + '_exp.bin'
        path_actual_out = PATH_TO_ACTUAL + test + '/' + fname + '.out'
        path_expected_out = PATH_TO_EXPECTED + test + '/' + fname + '_exp.out'
        #execute emulate
        error = execute_emulate(fname, path_emulate, path_bin, path_actual_out)
        #if error append FAILED for this test
        if error != False :
                results = 'FAILED'
                return [error, results, None, None]
        else:
            expected_out = read_output(path_expected_out)
            actual_out = read_output(path_actual_out)
            #remove spaces to compare outputs 
            expected_out_no_spaces = remove_spaces(expected_out)
            actual_out_no_spaces = remove_spaces(actual_out)
            #compare outputs
            if expected_out_no_spaces == actual_out_no_spaces:
                results = 'CORRECT'
            else:
                results = 'INCORRECT'
            return [error, results, expected_out, actual_out]

'''
Function to actually execute assembler 
Takes as input fname(all or path to specific test)
Executes run_tests.py which provides an out.json file with the results
We return True (run_tests.py executed without error) or the error (run_tests.py failed to execute due to error)
This function is called from assemble()
'''   
def execute_emulate(fname, path_emulate, path_bin, path_actual_out):
    #execute emulate
    try:
        command = [path_emulate, path_bin, path_actual_out]
        subprocess.check_output(command, stderr=subprocess.STDOUT, text=True)
        return False
    
    #if an error is caught 
    except subprocess.CalledProcessError as e: 
            if fname == 'all':
                return True
            else:
                return e

#function to read a file into a list
def read_output(path):
    with open(path) as f:
        lines = f.read().splitlines()
    f.close()
    return lines

#function to read a binary file into a list
def read_binary_output(path):
    with open(path, mode='rb') as f:
        lines = f.read()
    f.close()
    return lines

def remove_spaces(lines):
    new_lines = []
    for line in lines:
        new_lines.append(re.sub(' +', '', line))
    return new_lines
        
        

        

  