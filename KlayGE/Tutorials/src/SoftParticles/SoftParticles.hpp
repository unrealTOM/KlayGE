#ifndef _SOFT_PARTICLES_HPP
#define _SOFT_PARTICLES_HPP

#include <KlayGE/App3D.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/CameraController.hpp>

class SoftParticlesApp : public KlayGE::App3DFramework
{
public:
	SoftParticlesApp();

private:
	void OnCreate();
	void OnResize(KlayGE::uint32_t width, KlayGE::uint32_t height);

	void DoUpdateOverlay();
	KlayGE::uint32_t DoUpdate(KlayGE::uint32_t pass);

	void InputHandler(KlayGE::InputEngine const& sender, KlayGE::InputAction const& action);
	void TessFactorChangedHandler(KlayGE::UISlider const& sender);
	void PartitionTypeChangedHandler(KlayGE::UIComboBox const& sender);
	void WireframeHandler(KlayGE::UICheckBox const& sender);

	KlayGE::FontPtr font_;
	KlayGE::RenderModelPtr bezier_;

	KlayGE::TrackballCameraController tb_controller_;

	KlayGE::UIDialogPtr dialog_;
	float tess_factor_;

	int id_tessellation_factor_static_;
	int id_tessellation_factor_slider_;
	int id_partition_type_static_;
	int id_partition_type_combo_;
	int id_wireframe_;
};

#endif // _SOFT_PARTICLES_HPP