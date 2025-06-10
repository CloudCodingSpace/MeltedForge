@echo off
SetLocal EnableDelayedExpansion

pushd ..

for /R %%f in (*.vert) do (
	glslc -IMeltedForge/mfassets/shaders %%f -o %%f.spv
)

for /R %%f in (*.frag) do (
	glslc -IMeltedForge/mfassets/shaders %%f -o %%f.spv
)

popd

pause