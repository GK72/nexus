use nrs_meta::regular;

#[cfg(test)]
mod tests {
    use super::*;

    #[regular]
    struct Regular {
        value: i32
    }

    #[test]
    fn regular_type() {
        let x = Regular { value: 8 };
        let y = x;
        assert_eq!(x, y);
    }
}
