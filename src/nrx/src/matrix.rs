pub struct Matrix {
    data: Vec<i32>,
    n: usize,
    m: usize
}

use std::convert::TryInto;

impl Matrix {
    pub fn new(n: usize, m: usize) -> Self {
        Self { 
            data: vec![0; (n * m).try_into().unwrap()],
            n, m
        }
    }

    pub fn new_init(n: usize, m: usize, vec: Vec<i32>) -> Self {
        Self { 
            data: vec,
            n, m
        }
    }

    pub fn size(self) -> usize {
        self.data.len()
    }

    pub fn dim(self) -> (usize, usize) {
        (self.n, self.m)
    }

    pub fn sum_row(self, k: usize) -> i32 {
        let mut sum: i32 = 0;
        for i in 0..self.m {
            sum += self.data[i + k * self.n];
        }
        sum
    }

    pub fn sum_col(self, k: usize) -> i32 {
        let mut sum: i32 = 0;
        for i in 0..self.n {
            sum += self.data[i + k * self.m];
        }
        sum
    }

}
