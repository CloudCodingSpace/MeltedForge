# MFTest

This is a client program which is meant to test the features of the engine core for development purpose.  
This is test is written in C and it linked to MeltedForge through CMake. Note, that the code of this test may  
not be much readable or elegant, since its purpose is to just stress test the engine's core and find bugs, etc.

## Screenshots

 - **PBR *(Without Specular IBL)***
![pbr_screenshot](./screenshots/pbr_without_ibl.png)

 - **PBR *(With Specular IBL)***
![pbr_screenshot](./screenshots/pbr_with_ibl.png)

 - **Sponza scene (With PBR and IBL)**
![pbr_sponza](./screenshots/sponza_pbr.png)

 - **Multiple Entities with different mesh components *(With 4X MSAA on Forward render)***
![4x_msaa_multiple_entities_ss](./screenshots/4x_msaa_multiple_entities.png)