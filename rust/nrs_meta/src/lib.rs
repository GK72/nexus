use proc_macro::TokenStream;
use quote::quote;

#[proc_macro_attribute]
pub fn regular(_: TokenStream, item: TokenStream) -> TokenStream {
    let mut output: TokenStream = (quote! {
        #[derive(Eq, PartialEq, Copy, Clone, Debug)]
    })
    .into();

    output.extend(item);
    output
}
