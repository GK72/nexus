local attach_to_buffer = function(bufnr, pattern, command)
    vim.api.nvim_create_autocmd("BufWritePost", {
        group = vim.api.nvim_create_augroup("CppAutoSave", { clear = true }),
        pattern = pattern,
        callback = function()
            local append_data = function(_, data)
                if data then
                    vim.api.nvim_buf_set_lines(bufnr, -1, -1, false, data)
                end
            end
            vim.fn.jobstart(
                command,
                {
                    stdout_buffered = true,
                    on_stdout = append_data,
                    on_stderr = append_data,
                }
            )
        end
    })
end

vim.api.nvim_create_user_command(
    "AutoRun",
    function()
        local bufnr = tonumber(vim.fn.input "Bufnr: ")
        local pattern = vim.fn.input "Pattern: "
        local command = vim.fn.input "Command: "
        attach_to_buffer(bufnr, pattern, command)
    end,
    {}
)
