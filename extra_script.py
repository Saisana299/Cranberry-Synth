Import("env") # type: ignore
env.Append(LINKFLAGS=["-flto-partition=none"]) # type: ignore
