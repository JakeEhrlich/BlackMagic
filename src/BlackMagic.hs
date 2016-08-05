module BlackMagic(solve) where

import Data.List
import System.IO.Unsafe
import Foreign.Storable
import Foreign.C.String
import Foreign.C.Types
import Foreign.Ptr
import Foreign.ForeignPtr
import Foreign.ForeignPtr.Unsafe
import Foreign.Marshal.Array

foreign import ccall "newSolverState" c_newSolverState :: CString -> IO (Ptr ())
foreign import ccall "&freeSolverState" c_freeSolverState :: FunPtr (Ptr () -> IO ())
foreign import ccall "solverStateAddClause" c_solverStateAddClause :: Ptr () -> Ptr CInt -> IO ()
foreign import ccall "solverStateSolve" c_solverStateSolve :: Ptr () -> Ptr CInt -> IO CInt
foreign import ccall "solveTest" c_solveTest :: CString -> CInt -> IO ()
foreign import ccall "dynamic" mkFreeFun :: FunPtr (Ptr () -> IO ()) -> (Ptr () -> IO ())

newClause n (0:xs) = newClause (n + 1) xs
newClause n (1:xs) = (negate n) : newClause (n + 1) xs
newClause n (_:xs) = n : newClause (n + 1) xs
newClause _ []     = [0] --put the zero so it knows where to end

findSolution arr solver =  do
    ret <- c_solverStateSolve solver arr
    if ret /= 0
      then do ans <- peekArray0 0 arr
              --print ans
              let clause = newClause 1 ans
              --print clause
              pokeArray arr clause
              c_solverStateAddClause solver arr
              return $ Just ans
      else return Nothing

recurseSolutions fptr2 fptr1 arr vars solver = do
    touchForeignPtr fptr1 --putting this here because I'm afraid
    touchForeignPtr fptr2 --putting this here because I'm afraid
    sol <- findSolution arr solver
    case sol of
        Nothing -> return []
        Just sol' -> do sols <- unsafeInterleaveIO $ recurseSolutions fptr2 fptr1 arr vars solver
                        touchForeignPtr fptr1 --keep this alive, so it won't be collected
                        touchForeignPtr fptr2 --keep this alive, so it won't be collected
                        return (sol':sols)
    
mySolver vars str = do
    solver <- withCString str c_newSolverState --we need a solver pointer to be allocated
    if solver == nullPtr 
      then mkFreeFun c_freeSolverState solver >> return []
      else do farr <- mallocForeignPtrArray0 vars --allocate a foreign point to garbage collect this 
              let arr = unsafeForeignPtrToPtr farr --but we need the Ptr and withForeignPtr would break things    
              fsolver <- newForeignPtr c_freeSolverState solver --make a foreign point to garbage collect this    
              l <- recurseSolutions farr fsolver arr vars solver --lazily keep getting solutions but keep pointers slaive
              return l --return the list

satHeader vars sat = "p cnf " ++ show vars ++ " " ++ show (length sat) ++ "\n"
satClause lits = intercalate " " (map show lits) ++ " 0\n"
satBody (clause:clauses) = satClause clause ++ satBody clauses
satBody []               = ""
toDIMACS :: Int -> [[Int]] -> String
toDIMACS vars sat = satHeader vars sat ++ satBody sat

solve sat = unsafePerformIO (mySolver vars (toDIMACS vars sat))
    where vars = mymax (map (mymax . map abs) sat)
          mymax [] = 0
          mymax xs = maximum xs

myTest = withCString "this is a test" (\cstr -> c_solveTest cstr 10)



