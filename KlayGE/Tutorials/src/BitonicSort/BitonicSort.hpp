#ifndef _BITONIC_SORT_HPP
#define _BITONIC_SORT_HPP

#include <KlayGE/App3D.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/CameraController.hpp>

namespace
{
	class BitonicSortObject;
	using BitonicSortObjectPtr = std::shared_ptr<BitonicSortObject>;
}

class BitonicSortApp : public KlayGE::App3DFramework
{
public:
	BitonicSortApp();

private:
	void OnCreate();
	void OnResize(KlayGE::uint32_t width, KlayGE::uint32_t height);

	void DoUpdateOverlay();
	KlayGE::uint32_t DoUpdate(KlayGE::uint32_t pass);

	void InputHandler(KlayGE::InputEngine const& sender, KlayGE::InputAction const& action);

	KlayGE::FontPtr font_;
	BitonicSortObjectPtr bitonic_;

	KlayGE::TrackballCameraController tb_controller_;

	KlayGE::UIDialogPtr dialog_;
	float tess_factor_;
};

#endif // _BITONIC_SORT_HPP