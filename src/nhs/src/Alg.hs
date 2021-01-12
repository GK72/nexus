-- Algorithms module

module Alg where

divisors :: Integral a => a -> [a]
divisors x = map fst
           $ filter
                (\(_, r) -> r == 0)
                [(y, x `mod` y) | y <- [1..x]]

-- Return a list of every nth element starting from the first element
-- every 5 [0..10] == [0,5]
every :: Int -> [a] -> [a]
every n [] = []
every n xs = head xs : every n (drop n xs)

count p xs = length $ filter p xs

-- polynom xs x = sum $ map (\(deg, coeff) -> x ^ deg * coeff) $ zip [0..length xs - 1] xs
{- |
   E.g: x^3 + 2x^2 + 10x + 0
   polynom [0, 10, 2, 1] 2 == 36
-}
polynom xs x = sum $ zipWith (\deg coeff -> x ^ deg * coeff) [0..length xs - 1] xs

lookup' k d
    | null result = Nothing
    | otherwise = Just $ snd $ head result
    where
        result = filter (\x -> fst x == k) d
