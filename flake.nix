{
    description = "The titanium game engine and general development library";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/23.05";
        futils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, futils }:
        futils.lib.eachDefaultSystem( system: let pkgs = import nixpkgs { inherit system; };
            in {
                devShell = pkgs.mkShell rec {
                    nativeBuildInputs = [
                        pkgs.cmake

                        pkgs.clang_16
                        pkgs.clang-tools_16
                        pkgs.emscripten

                        pkgs.gdb

                        # dawn dependencies
                        pkgs.python3
                        pkgs.pkg-config
                    ];

                    buildInputs = [
                        pkgs.vulkan-headers
                        pkgs.vulkan-tools
                        pkgs.vulkan-validation-layers
                        pkgs.SDL2

                        # dawn dependencies
                        pkgs.xorg.libXrandr
                        pkgs.xorg.libXinerama
                        pkgs.xorg.libXcursor
                        pkgs.xorg.libXi
                        pkgs.wayland-protocols
                        #wayland-client
                    ];

                    # we don't load vulkan correctly without this
                    LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib/";
                };
            }
        );
}
