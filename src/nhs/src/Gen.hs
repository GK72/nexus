-- Generators module

module Gen where

-- The simple and inefficient method
fib' 0 = 0
fib' 1 = 1
fib' 2 = 1
fib' n = fib' (n-1) + fib' (n-2)

-- The fast method
fibSeq = 1 : 1 : [a + b | (a, b) <- zip fibSeq (tail fibSeq)]
fib n = last $ take n fibSeq

-- A simple list comprehension with condition
evens = [x | x <- [1..], even x]

-- altSeries s = map (\(a, b) -> a * b) $ zip s (iterate ((-1) *) 1)
-- altSeries s = zipWith (curry (\(a, b) -> a * b)) s (iterate ((-1) *) 1)
-- altSeries s = zipWith (\a b -> a * b) s (iterate ((-1) *) 1)
altSeries s = zipWith (*) s (iterate ((-1) *) 1)
piLeibniz n = sum $ map (1/) $ take n $ altSeries [1,3..]
pi' n = piLeibniz n * 4
