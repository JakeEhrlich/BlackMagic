module Paths_BlackMagic (
    version,
    getBinDir, getLibDir, getDataDir, getLibexecDir,
    getDataFileName, getSysconfDir
  ) where

import qualified Control.Exception as Exception
import Data.Version (Version(..))
import System.Environment (getEnv)
import Prelude

catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
catchIO = Exception.catch

version :: Version
version = Version [0,1,0,0] []
bindir, libdir, datadir, libexecdir, sysconfdir :: FilePath

bindir     = "/Users/jakeehrlich/BlackMagic/.stack-work/install/x86_64-osx/lts-6.10/7.10.3/bin"
libdir     = "/Users/jakeehrlich/BlackMagic/.stack-work/install/x86_64-osx/lts-6.10/7.10.3/lib/x86_64-osx-ghc-7.10.3/BlackMagic-0.1.0.0-5wLmdDMccf24H4NCej5G0q"
datadir    = "/Users/jakeehrlich/BlackMagic/.stack-work/install/x86_64-osx/lts-6.10/7.10.3/share/x86_64-osx-ghc-7.10.3/BlackMagic-0.1.0.0"
libexecdir = "/Users/jakeehrlich/BlackMagic/.stack-work/install/x86_64-osx/lts-6.10/7.10.3/libexec"
sysconfdir = "/Users/jakeehrlich/BlackMagic/.stack-work/install/x86_64-osx/lts-6.10/7.10.3/etc"

getBinDir, getLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath
getBinDir = catchIO (getEnv "BlackMagic_bindir") (\_ -> return bindir)
getLibDir = catchIO (getEnv "BlackMagic_libdir") (\_ -> return libdir)
getDataDir = catchIO (getEnv "BlackMagic_datadir") (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "BlackMagic_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "BlackMagic_sysconfdir") (\_ -> return sysconfdir)

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir ++ "/" ++ name)
