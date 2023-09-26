Low-level memory reading plugin for DFHack
==========================================

Remote API
----------

 - `llmemoryreader::GetInfo`: `dfproto::EmptyMessage` → `dfproto::llmemoryreader::Info` get information about current process: version string, PE timestamp or MD5 checksum indentifying this version, and rebase offset for the main module.
 - `llmemoryreader::ReadRaw`: `dfproto::llmemoryreader::ReadRawIn` → `dfproto::llmemoryreader::ReadRawOut` reads `length` bytes of raw data at `address`.
 - `llmemoryreader::ReadRawV`: `dfproto::llmemoryreader::ReadRawVIn` → `dfproto::llmemoryreader::ReadRawVOut` do several `ReadRaw` in a single call.
