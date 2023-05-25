################################################ imports #################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

#pip install Flask
from flask import Flask, render_template
from helper_functions import *
import shutil

app = Flask(__name__, template_folder='templates')

############################################## templates #################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
##########################################################################################################

#homepage
@app.route('/')
def index():
    return render_template('index.html')

#page to generate tests
@app.route('/compile-solution')
def compile_solution():
    #if solution directory is not found
    check_path = path_exists([PATH_TO_SOLUTION])
    if check_path:
        return render_template('compile_solution.html', path_error = True, title='Compile Solution - Directory Error')
    
    #if makefile is not found
    path_makefile = find_path(fname='Makefile')
    if path_makefile == None:
        return render_template('compile_solution.html', makefile_error = True, title='Compile Solution - Makefile Error')
    
    #otherwise execute makefile
    result = run_makefile(path_makefile)
    if result == True:
        return render_template('compile_solution.html', execution = 'Makefile successfully compiled!', title='Compile Solution')
    else:
        return render_template('compile_solution.html', execution_error = result, title='Compile Solution - Execution Error')

#page to find specific test to assemble
@app.route('/find-test-<fname>')
def find_test(fname):
    names = []
    for test in TESTS:
        for i in range(0,20):
            test_name = test+str(i)
            if fname == 'assemble':
                names.append(test_name)
            if fname == 'emulate' and test_name not in ['ldr0', 'ldr1', 'ldr13', 'ldr15', 'ldr18', 'ldr19', 'ldr2', 'ldr3', 'ldr4', 'str0', 'str10', 'str14', 'str15', 'str16', 'str18', 'str2', 'str3', 'str4', 'str5', 'str8']:
                names.append(test + str(i))
    return render_template('find_test.html', names = names, fname = fname, title = 'List of Tests')

#page to assemble all tests
@app.route('/assembler-results')
def assemble_all_tests():
    #check if ./tests/test_cases and ./solution directories exist
    check_path = path_exists([PATH_TO_TESTS, PATH_TO_SOLUTION, PATH_TO_EXPECTED])
    if check_path:
        return render_template('assemble_all_tests.html', directory_error = check_path, title = 'Assembler Results - Directory Error')

    #check if all test case files exist
    check_files = file_exists(fname='all', path= PATH_TO_TESTS, file_ending='.s')
    if check_files:
        return render_template('assemble_all_tests.html', file_errors = check_files, title = 'Assembler Results - File Error')    
    
    #check if all expected .bin files exist
    check_files = file_exists(fname='all', path= PATH_TO_EXPECTED, file_ending='_exp.bin')
    if check_files:
        return render_template('assemble_all_tests.html', file_errors = check_files, title = 'Assembler Results - File Error')    
    
    #check that assembler exists.
    path_assembler = find_path(fname='assemble')
    if path_assembler == None:
        return render_template('assemble_all_tests.html', assembler_error = True, title='Assembler Results - File Error')
    
    #if everything is found
    else:
        res = assemble('all', path_assembler)
        return render_template('assemble_all_tests.html', results = res[1], files = res[0], num_cor = res[1].count('CORRECT'), num_inc = res[1].count('INCORRECT'), num_fail = res[1].count('FAILED'), total_tests= len(res[1]), title = 'Assembler Results')

#page to assemble a single tests
@app.route('/assembler-results/<fname>')
def assemble_individual(fname):
    #check if ./tests/test_cases and ./solution directories exist
    check_path = path_exists([PATH_TO_TESTS, PATH_TO_SOLUTION, PATH_TO_EXPECTED])
    if check_path:
        return render_template('assemble_individual.html', directory_error = check_path, title = fname + ' Assembler Results - Directory Error')

    #check if test case exists
    check_files = file_exists(fname=fname, path= PATH_TO_TESTS, file_ending='.s')
    if check_files:
        return render_template('assemble_individual.html', file_errors = check_files, title = fname + ' Assembler Results - File Error')    
    
    #check expected .bin file exists
    check_files = file_exists(fname=fname, path= PATH_TO_EXPECTED, file_ending='_exp.bin')
    if check_files:
        return render_template('assemble_individual.html', file_errors = check_files, title = fname + ' Assembler Results - File Error')    
    
    #check that assembler exists.
    path_assembler = find_path(fname='assemble')
    if path_assembler == None:
        return render_template('assemble_individual.html', assembler_error = True, title= fname + ' Assembler Results - File Error')
    
    #if everything is found
    else:
        res = assemble(fname, path_assembler)
        #if no error
        if res[0] == False:
            return render_template('assemble_individual.html', results = res[1], files = fname, actual_output = res[3], expected_output = res[2], title = fname + ' Assembler Results')
        else:
            return render_template('assemble_individual.html', execution_error = res[0], result = res[1], fname=fname, title = fname + ' Assembler Execution Error')


 #page to emulate all tests
@app.route('/emulator-results')
def emulate_all_tests():
    #check if ./tests/test_cases and ./solution directories exist
    check_path = path_exists([PATH_TO_TESTS, PATH_TO_SOLUTION, PATH_TO_EXPECTED])
    if check_path:
        return render_template('emulate_all_tests.html', directory_error = check_path, title = 'Emulator Results - Directory Error')

    #check if all test case files exist
    check_files = file_exists(fname='all', path= PATH_TO_TESTS, file_ending='.s')
    if check_files:
        return render_template('emulate_all_tests.html', file_errors = check_files, title = 'Emulator Results - File Error')    
    
    #check if all expected .bin files exist
    check_files = file_exists(fname='all', path= PATH_TO_EXPECTED, file_ending='_exp.bin')
    if check_files:
        return render_template('emulate_all_tests.html', file_errors = check_files, title = 'Emulator Results - File Error')    
    
    #check that assembler exists.
    path_emulator = find_path(fname='emulate')
    if path_emulator == None:
        return render_template('emulate_all_tests.html', emulator_error = True, title='Emulator Results - File Error')
    
    #if everything is found
    else:
        res = emulate('all', path_emulator)
        return render_template('emulate_all_tests.html', results = res[1], files = res[0], num_cor = res[1].count('CORRECT'), num_inc = res[1].count('INCORRECT'), num_fail = res[1].count('FAILED'), total_tests= len(res[1]), title = 'Emulator Results')

   

#page to emulate a specific test
@app.route('/emulator-results/<fname>')
def emulate_individual(fname):
       #check if ./tests/test_cases and ./solution directories exist
    check_path = path_exists([PATH_TO_TESTS, PATH_TO_SOLUTION, PATH_TO_EXPECTED])
    if check_path:
        return render_template('emulate_individual.html', directory_error = check_path, title = fname + ' Emulator Results - Directory Error')

    #check if test case exists
    check_files = file_exists(fname=fname, path= PATH_TO_TESTS, file_ending='.s')
    if check_files:
        return render_template('emulate_individual.html', file_errors = check_files, title = fname + ' Emulator Results - File Error')    
    
    #check expected .bin file exists
    check_files = file_exists(fname=fname, path= PATH_TO_EXPECTED, file_ending='_exp.bin')
    if check_files:
        return render_template('emulate_individual.html', file_errors = check_files, title = fname + ' Emulator Results - File Error')    
    
    #check that assembler exists.
    path_emulate = find_path(fname='emulate')
    if path_emulate == None:
        return render_template('emulate_individual.html', emulator_error = True, title= fname + ' Emulator Results - File Error')
    
    #if everything is found
    else:
        res = emulate(fname, path_emulate)
        #if no error
        if res[0] == False:
            return render_template('emulate_individual.html', results = res[1], fname = fname, actual_output = res[3], expected_output = res[2], title = fname + ' Emulator Results')
        else:
            return render_template('emulate_individual.html', execution_error = res[0], result = res[1], fname=fname, title = fname + ' Emulator Execution Error')

################################################ main ####################################################
#                                                                                                        #  
#                                                                                                        #   
#                                                                                                        #   
########################################################################################################## 

if __name__ == "__main__":
   app.run(debug=True)