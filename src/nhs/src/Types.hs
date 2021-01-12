module Types where

data Month = January
           | February
           | March
           | April
           | May
           | June
           | July
           | August
           | September
           | October
           | November
           | December
           deriving (Show, Eq, Ord, Enum, Bounded)

data Date = Date Int Month Int
            deriving (Eq, Ord)

instance Show Date where
    show (Date y m d) = show y ++ "." ++ show m ++ "." ++ show d

addMonth :: Date -> Date
addMonth (Date y m d)
    | m == December = Date (y + 1) January d
    | otherwise = Date y (succ m) d


data Symbol = HUF | EUR | USD deriving (Eq)
instance Show Symbol where
    show HUF = "Hungarian Forint"
    show EUR = "Euro"
    show USD = "US Dollar"

data Currency = Currency { symbol :: Symbol,
                           value :: Int
                         } deriving (Eq, Show)

-- Usage: currency HUF $ Currency EUR 10
currency HUF (Currency sym val)
    | sym == EUR = Currency HUF (val * 380)
    | sym == USD = Currency HUF (val * 300)

data Color = BLACK | WHITE | RED | BLUE | GREEN deriving (Show)

-- Record
-- Usage: color (Car "Mitsubishi" 1998 GREEN)
data Car = Car { model :: String,
                 year :: Int,
                 color :: Color }


data Tree a = EmptyTree | Node a (Tree a) (Tree a) deriving (Show, Read, Eq)

treeSingle :: a -> Tree a
treeSingle x = Node x EmptyTree EmptyTree

-- Usage: foldr treeInsert EmptyTree [5,3,7,9,13,52]
treeInsert :: Ord a => a -> Tree a -> Tree a
treeInsert x EmptyTree = treeSingle x
treeInsert x (Node a lhs rhs)
    | x == a = Node x lhs rhs
    | x  < a = Node a (treeInsert x lhs) rhs
    | x  > a = Node a lhs (treeInsert x rhs)

treeElem :: Ord a => a -> Tree a -> Bool
treeElem x EmptyTree = False
treeElem x (Node a lhs rhs)
    | x == a = True
    | x  < a = treeElem x lhs
    | x  > a = treeElem x rhs

