import Distribution.Simple
import Distribution.System
import Distribution.Simple.Program.Run
import Distribution.Verbosity

--try and find the correct file ending for the shared librar
fileEnding Windows = "dll"
fileEnding OSX = "dylib"
fileEnding Ghcjs = error "you need to handle this a tad differently"
fileEnding _ = "so"

--build the shared library before doing any else
--attempts to build the shared library in crossplatform way
--on windows this assumes MinGW or Cygwin
--on everything it dosn't make any assumptions really
nativePreConf args config = do
    let prog = emptyProgramInvocation { 
        progInvokePath = "g++", 
        progInvokeArgs = ["-fPIC", "-O3", "-shared",
                          "minisat/Main.cpp", "minisat/Solver.cpp",
                          "-o", "minisat/libminisat." ++ fileEnding buildOS] }
    runProgramInvocation normal prog
    preConf simpleUserHooks args config

--assumes that you have emscipten installed
jsPreConf args config = do
    let prog = emptyProgramInvocation { 
        progInvokePath = "emcc", 
        progInvokeArgs = ["-O3",
                          "minisat/Main.cpp", "minisat/Solver.cpp",
                          "EXPORTED_FUNCTIONS=\"['_newSolverState','_freeSolverState','_solverStateAddClause','_solverStateSolve']\"",
                          "-o", "minisat/libminisat.js"] }
    runProgramInvocation normal prog
    preConf simpleUserHooks args config

generalPreConf = case buildOS of
    Ghcjs -> jsPreConf
    _     -> nativePreConf

main = defaultMainWithHooks $ simpleUserHooks {preConf = generalPreConf}
