#[macro_use]
mod macros {
    #[macro_export]
    macro_rules! assert_comp {
        ($lhs:expr, $rhs:expr) => {
            assert_eq!($lhs, $rhs,
                "\nExpected : {:?}\
                 \nActual   : {:?}\n",
                $lhs, $rhs
            );
        };
        ($lhs:expr, $rhs:expr, $func:ident) => {
            assert!($func($lhs, $rhs, 0.00001),
                "\nExpected : {:.5?}\
                 \nActual   : {:.5?}\n",
                $lhs, $rhs
            );
        };
    }
}
