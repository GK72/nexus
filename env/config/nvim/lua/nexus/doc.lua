-- Part of Nexus Library
--
-- Live preview of AsciiDoc through `tools/doc.sh --serve`. A buffer that belongs to a book (a
-- folder with one `.adoc`, the spine, and a `pages/` folder) serves the whole book, so the pages
-- carry their navigation; any other `.adoc` serves alone. The browser opens on the page of the
-- buffer once it has rendered; the server re-renders on every write until stopped.
--
--   :DocServe [PORT]   serve the book or file of the buffer, or show it if already served
--   :DocStop           stop the server
--   :DocLog            the output of the server
--
-- Buffer maps in AsciiDoc files: <localleader>p serves, <localleader>q stops.
--
-- Settings: `vim.g.nexus_doc_script` (default: `tools/doc.sh` of the Nexus checkout this config
-- lives in, else `doc.sh` on PATH), `vim.g.nexus_doc_port` (default: 8000).

local M = {}

local server = nil -- { job, target, port, page, ready }
local log_lines = {}

local function script()
    if vim.g.nexus_doc_script then
        return vim.fn.expand(vim.g.nexus_doc_script)
    end
    -- The config is `env/config/nvim` of the checkout, usually through a symlink
    local config = vim.fn.resolve(vim.fn.stdpath("config"))
    local candidate = vim.fs.normalize(config .. "/../../../tools/doc.sh")
    if vim.fn.executable(candidate) == 1 then
        return candidate
    end
    local found = vim.fn.exepath("doc.sh")
    return found ~= "" and found or nil
end

-- The spine of the book in folder `dir`, as `tools/doc.sh` finds it: its only `.adoc`, with a
-- `pages/` folder beside it.
local function spine_of(dir)
    if vim.fn.isdirectory(dir .. "/pages") == 0 then
        return nil
    end
    local adocs = vim.fn.glob(dir .. "/*.adoc", true, true)
    adocs = vim.tbl_filter(function(path) return vim.fn.filereadable(path) == 1 end, adocs)
    return #adocs == 1 and adocs[1] or nil
end

-- What to serve for `file`: `{ target, page }`, the argument of the script and the path of the
-- buffer's page under the served folder.
local function resolve(file)
    local dir = vim.fs.dirname(file)
    local name = vim.fn.fnamemodify(file, ":t:r")

    local spine = spine_of(dir)
    if spine then
        -- The spine itself, or the only `.adoc` beside `pages/`
        return { target = dir, page = vim.fn.fnamemodify(spine, ":t:r") .. "/index.html" }
    end
    if vim.fs.basename(dir) == "pages" then
        local book = vim.fs.dirname(dir)
        spine = spine_of(book)
        if spine then
            local book_name = vim.fn.fnamemodify(spine, ":t:r")
            -- A partial (`_*.adoc`) has no page of its own: show the book
            local page = name:sub(1, 1) == "_" and "index" or name
            return { target = book, page = book_name .. "/" .. page .. ".html" }
        end
    end
    return { target = file, page = name .. ".html" }
end

local function url(page)
    return string.format("http://127.0.0.1:%d/%s", server.port, page)
end

local function show(page)
    server.page = page
    if server.ready then
        vim.ui.open(url(page))
    end
end

local function on_output(_, data)
    for _, line in ipairs(data) do
        if line ~= "" then
            table.insert(log_lines, line)
            if #log_lines > 1000 then
                table.remove(log_lines, 1)
            end
            if server and not server.ready and line:find("serving ") then
                server.ready = true
                vim.notify("doc: serving " .. url(server.page))
                vim.ui.open(url(server.page))
            elseif line:find("^asciidoctor: ") or line:find(": failed: ") then
                vim.notify("doc: " .. line, vim.log.levels.WARN)
            end
        end
    end
end

function M.stop()
    if server then
        vim.fn.jobstop(server.job)
        server = nil
    end
end

function M.serve(port)
    local file = vim.api.nvim_buf_get_name(0)
    if file == "" or not file:match("%.adoc$") then
        vim.notify("doc: not an AsciiDoc file", vim.log.levels.ERROR)
        return
    end
    local exe = script()
    if not exe then
        vim.notify("doc: tools/doc.sh not found (set vim.g.nexus_doc_script)", vim.log.levels.ERROR)
        return
    end

    local spec = resolve(vim.fs.normalize(vim.fn.fnamemodify(file, ":p")))
    port = tonumber(port) or (server and server.port) or vim.g.nexus_doc_port or 8000
    if server and server.target == spec.target and server.port == port then
        show(spec.page)
        return
    end
    M.stop()
    if vim.bo.modified then
        vim.cmd.write()
    end

    log_lines = {}
    local job = vim.fn.jobstart({ exe, "--serve=" .. port, spec.target }, {
        -- The output goes under the git root of the document's project
        cwd = vim.fs.dirname(file),
        on_stdout = on_output,
        on_stderr = on_output,
        on_exit = function(id, code)
            if server and server.job == id then
                server = nil
                if code ~= 0 and code ~= 143 then
                    vim.notify("doc: the server exited (" .. code .. "), see :DocLog", vim.log.levels.ERROR)
                end
            end
        end,
    })
    if job <= 0 then
        vim.notify("doc: cannot run " .. exe, vim.log.levels.ERROR)
        return
    end
    server = { job = job, target = spec.target, port = port, page = spec.page, ready = false }
    vim.notify("doc: rendering " .. vim.fn.fnamemodify(spec.target, ":~:."))
end

function M.log()
    vim.cmd.new()
    vim.bo.buftype = "nofile"
    vim.bo.bufhidden = "wipe"
    vim.api.nvim_buf_set_lines(0, 0, -1, false, log_lines)
end

function M.setup()
    vim.api.nvim_create_user_command("DocServe", function(opts) M.serve(opts.args) end,
        { nargs = "?", desc = "Serve the AsciiDoc book or file of the buffer" })
    vim.api.nvim_create_user_command("DocStop", M.stop, { desc = "Stop the AsciiDoc server" })
    vim.api.nvim_create_user_command("DocLog", M.log, { desc = "Show the AsciiDoc server output" })

    local group = vim.api.nvim_create_augroup("NexusDoc", {})
    vim.api.nvim_create_autocmd("VimLeavePre", { group = group, callback = M.stop })
    vim.api.nvim_create_autocmd("FileType", {
        group = group,
        pattern = "asciidoc",
        callback = function(args)
            vim.keymap.set("n", "<localleader>p", M.serve, { buffer = args.buf, desc = "Serve the document" })
            vim.keymap.set("n", "<localleader>q", M.stop, { buffer = args.buf, desc = "Stop serving" })
        end,
    })
end

return M
