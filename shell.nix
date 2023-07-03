{ pkgs ? import <nixpkgs> { } }:
with pkgs;
mkShell {
	nativeBuildInputs = [
		clang_16
		clang-tools_16
		cmake
		gdb
		emscripten
	];

	buildInputs = [
		vulkan-headers
		vulkan-tools
		vulkan-validation-layers
		SDL2
	];

	packages = [ vulkan-loader ];
	LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib/";
}
