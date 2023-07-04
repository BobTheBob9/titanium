{ pkgs ? import <nixpkgs> { } }:
with pkgs;
mkShell {
	nativeBuildInputs = [
		clang_16
		clang-tools_16
		cmake
		gdb
		emscripten

		# dawn dependencies
		python3
		pkg-config
	];

	buildInputs = [
		vulkan-headers
		vulkan-tools
		vulkan-validation-layers
		SDL2

		# dawn dependencies
		xorg.libXrandr
		xorg.libXinerama
		xorg.libXcursor
		xorg.libXi
		wayland-protocols
		#wayland-client
	];

	packages = [ vulkan-loader ];
	LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib/";
}
