#[cfg(test)]
mod test_matrix {
    use crate::matrix::Matrix;

    #[test]
    fn create_matrix() {
        let m = Matrix::new(3, 3);
        assert_eq!(m.size(), 9);
    }

    #[test]
    fn sum_row() {
        let m = Matrix::new_init(3, 2, vec![1, 2, 3, 4, 5, 6]);
        assert_eq!(m.sum_row(0), 3);
    }

    #[test]
    fn sum_col() {
        let m = Matrix::new_init(3, 2, vec![1, 2, 3, 4, 5, 6]);
        assert_eq!(m.sum_col(0), 9);
    }


}

