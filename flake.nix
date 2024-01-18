{
    description = "The titanium game engine and general development library";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/23.05";
        futils.url = "github:numtide/flake-utils";

        # for local simultaneous dev, you can override this: e.g. nix build --override-input libtitanium path:/home/bobthebob/repos/libtitanium
        libtitanium.url = "github:BobTheBob9/libtitanium";
    };

    outputs = { self, nixpkgs, futils, libtitanium }:
        futils.lib.eachDefaultSystem( system: let pkgs = import nixpkgs { inherit system; };
            in {
                packages = {
                    default = pkgs.llvmPackages_16.stdenv.mkDerivation {
                        name = "Titanium";
                        src = ./.;

                        nativeBuildInputs = [
                            pkgs.cmake
                            #pkgs.emscripten
                        ];

                        buildInputs = [
                            pkgs.SDL2
                            pkgs.openalSoft
                            pkgs.assimp
                            pkgs.stb
                        ];

                        env.LIBTITANIUM = libtitanium.outPath;
                    };
                };

                devShell = pkgs.mkShell.override { stdenv = pkgs.clang16Stdenv; } {
                    inputsFrom = [ self.packages.${system}.default ];

                    # dev tools
                    packages = [
                        pkgs.gdb # despite using all llvm tools here, i really don't like lldb
                        pkgs.clang-tools_16
                        pkgs.pkg-config
                        pkgs.compdb
                    ];

                    # we don't load vulkan correctly at runtime without this
                    # TODO: fix
                    LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib/";
                    LIBTITANIUM = libtitanium.outPath;
                };
            }
        );
}
