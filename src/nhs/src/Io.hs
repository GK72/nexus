module Io where

import System.IO

sayHello = do
    putStrLn "Name:"
    name <- getLine
    putStrLn $ "Hello " ++ name

writeFile = do
    outf <- openFile "test.txt" WriteMode
    hPutStrLn outf ("Test")
    hClose outf

readFile = do
    inf <- openFile "test.txt" ReadMode
    contents <- hGetContents inf
