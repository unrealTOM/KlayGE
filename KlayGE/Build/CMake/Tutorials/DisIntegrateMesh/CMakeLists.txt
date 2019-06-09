DOWNLOAD_DEPENDENCY("KlayGE/Tutorials/media/DisIntegrateMesh/grass.dds" "99497d6d0617e14eecd8c25e16e2f31eb9011195")
DOWNLOAD_DEPENDENCY("KlayGE/Tutorials/media/DisIntegrateMesh/particle.dds" "80a47e13017dfd960f65b153313629c34d3ab0a3")
DOWNLOAD_DEPENDENCY("KlayGE/Tutorials/media/DisIntegrateMesh/terrain_height.dds" "a9501482cf9671d67fb8341f48af5ac415fdf7e6")
DOWNLOAD_DEPENDENCY("KlayGE/Tutorials/media/DisIntegrateMesh/terrain_normal.dds" "c677f9f2e75f474d47c0c70da892aea604cda620")

SET(SOURCE_FILES
	${KLAYGE_PROJECT_DIR}/Tutorials/src/DisIntegrateMesh/DisIntegrateMesh.cpp
)

SET(HEADER_FILES
	${KLAYGE_PROJECT_DIR}/Tutorials/src/DisIntegrateMesh/DisIntegrateMesh.hpp
)

SET(EFFECT_FILES
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/DisIntegrateMesh.fxml
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/Terrain.fxml
)

SET(POST_PROCESSORS
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/Blend.ppml
)

SET(UI_FILES
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/DisIntegrateMesh.uiml
)

SET(CONTENT_FILES
	${KLAYGE_PROJECT_DIR}/bin/KlayGE.cfg
	${KLAYGE_PROJECT_DIR}/media/Fonts/gkai00mp.kfont
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/Copy.ppml
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/LensEffects.ppml
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/PostToneMapping.ppml
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/Resizer.ppml
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/SMAA.ppml
	${KLAYGE_PROJECT_DIR}/media/PostProcessors/ToneMapping.ppml
	${KLAYGE_PROJECT_DIR}/media/RenderFX/Blur.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/Copy.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/Font.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/LensEffects.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/PostToneMapping.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/Resizer.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/SMAA.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/SumLum.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/ToneMapping.kfx
	${KLAYGE_PROJECT_DIR}/media/RenderFX/UI.kfx
	${KLAYGE_PROJECT_DIR}/media/Textures/2D/powered_by_klayge.dds
	${KLAYGE_PROJECT_DIR}/media/Textures/2D/SMAAAreaTex.dds
	${KLAYGE_PROJECT_DIR}/media/Textures/2D/SMAASearchTex.dds
	${KLAYGE_PROJECT_DIR}/media/Textures/2D/ui.dds
	${KLAYGE_PROJECT_DIR}/media/Textures/3D/color_grading.dds
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/GPUParticleSystem.kfx
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/grass.dds
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/particle.dds
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/Terrain.kfx
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/terrain_height.dds
	${KLAYGE_PROJECT_DIR}/Tutorials/media/DisIntegrateMesh/terrain_normal.dds
)

IF(KLAYGE_PLATFORM_WINDOWS_STORE)
	SET(PACKAGE_GUID "AD5AC2EF-D022-437B-A44A-5338E7A38C83")
ENDIF()

SETUP_TUTORIAL(DisIntegrateMesh)
