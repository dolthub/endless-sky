For the official Endless-Sky project and readme visit [https://github.com/endless-sky/endless-sky/](https://github.com/endless-sky/endless-sky/).
This repository is for a demonstration purposes only with the purpose of showing how a project could use dolt to manage
its data such as configuration. This demo currently only works on Intel macs.

## Building

### macOS

Start by cloning the dolthub endless-sky fork:

```sh
git clone git@github.com:dolthub/endless-sky.git
```

You will need some third-party packages to build endless-sky. The easiest way is to use the included script to install them from a binary cache into an isolated environment and build from within that environment. Start by install `nix`:

```sh
sh <(curl -L https://nixos.org/nix/install) --daemon
```

`cd` into the checked out source:

```sh
cd endless-sky
```

Then clone the endless-sky database from dolthub. I like to have the source and data together so I clone the database
into the newly created endless-sky folder.  The database is expected to be named `datadb` so you can clone it like so:

`dolt clone dolthub/endless-sky datadb`

You install the build dependencies and build the source with:

```bash
nix develop
```

It will start a shell with all the dependencies installed, and run the build with some variation of:

```bash
cmake .
cmake --build .
```

## Running

The demo currently makes use of unreleased `dolt` features. Install `dolt` from
Brian's branch: https://github.com/dolthub/dolt/tree/bh/hashof_table

```sh
cd ~/src/dolt/go
git checkout bh/hashof_table
go install ./cmd/dolt
```

Start a dolt sql-server instance serving the database cloned into the
`endless-sky/datadb` folder with the user `dolt` (no password):

```bash
cd ~/src/endless-sky/datadb
dolt sql-server -H127.0.0.1 -udolt
```

Then run the game:

```bash
cd ~/src/endless-sky
./build/macos/Debug/endless-sky
```
