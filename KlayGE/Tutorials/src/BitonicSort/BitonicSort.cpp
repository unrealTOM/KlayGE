#include <KlayGE/KlayGE.hpp>
#include <KFL/CXX17/iterator.hpp>
#include <KFL/Math.hpp>
#include <KFL/Half.hpp>
#include <KFL/Util.hpp>
#include <KlayGE/Camera.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/FrameBuffer.hpp>
#include <KlayGE/KlayGE.hpp>
#include <KlayGE/RenderEffect.hpp>
#include <KlayGE/RenderEngine.hpp>
#include <KlayGE/RenderSettings.hpp>
#include <KlayGE/Renderable.hpp>
#include <KlayGE/RenderableHelper.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/SceneNodeHelper.hpp>
#include <KlayGE/UI.hpp>

#include <KlayGE/InputFactory.hpp>
#include <KlayGE/RenderFactory.hpp>

#include <algorithm>
#include <sstream>
#include <vector>
#include <random>

#include "SampleCommon.hpp"
#include "BitonicSort.hpp"

using namespace std;
using namespace KlayGE;

namespace
{
	// The number of elements to sort is limited to an even power of 2
	// At minimum 8,192 elements - BITONIC_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE
	// At maximum 262,144 elements - BITONIC_BLOCK_SIZE * BITONIC_BLOCK_SIZE
	const uint32_t NUM_ELEMENTS = 512 * 512;
	const uint32_t BITONIC_BLOCK_SIZE = 512;
	const uint32_t TRANSPOSE_BLOCK_SIZE = 16;
	const uint32_t MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
	const uint32_t MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

	std::vector<uint32_t> data(NUM_ELEMENTS);
	std::vector<uint32_t> results(NUM_ELEMENTS);

	ranlux24_base gen;
	uniform_int_distribution<> random_dis(0, 100000);

	uint32_t SampleRand()
	{
		return (uint32_t)random_dis(gen);
	}

	class BitonicSortObject : public SceneNode
	{
	public:
		BitonicSortObject() : SceneNode(L"BitonicSortObject", SOA_Invisible)
		{
			RenderFactory& rf = Context::Instance().RenderFactoryInstance();

			effect_ = SyncLoadRenderEffect("BitonicSort.fxml");
			technique_transpose_ = effect_->TechniqueByName("Transpose");
			technique_bitonic_ = effect_->TechniqueByName("Bitonic");

			uint32_t access_hint = EAH_GPU_Read | EAH_GPU_Write | EAH_GPU_Unordered | EAH_GPU_Structured;

			buffer1_ = rf.MakeVertexBuffer(BU_Dynamic, access_hint, NUM_ELEMENTS * sizeof(uint32_t), nullptr, sizeof(uint32_t));
			buffer1_uav_ = rf.MakeBufferUav(buffer1_, EF_R32UI);
			buffer1_srv_ = rf.MakeBufferSrv(buffer1_, EF_R32UI);

			buffer2_ = rf.MakeVertexBuffer(BU_Dynamic, access_hint, NUM_ELEMENTS * sizeof(uint32_t), nullptr, sizeof(uint32_t));
			buffer2_uav_ = rf.MakeBufferUav(buffer2_, EF_R32UI);
			buffer2_srv_ = rf.MakeBufferSrv(buffer2_, EF_R32UI);

			result_cpu_buffer_ = rf.MakeVertexBuffer(BU_Dynamic, EAH_CPU_Read, buffer1_->Size(), nullptr);
		}

		void SetConstants(uint32_t iLevel, uint32_t iLevelMask, uint32_t iWidth, uint32_t iHeight)
		{
			*(effect_->ParameterByName("g_iLevel")) = iLevel;
			*(effect_->ParameterByName("g_iLevelMask")) = iLevelMask;
			*(effect_->ParameterByName("g_iWidth")) = iWidth;
			*(effect_->ParameterByName("g_iHeight")) = iHeight;
		}

		void Update(float /*app_time*/, float /*elapsed_time*/)
		{
			RenderEngine& re = Context::Instance().RenderFactoryInstance().RenderEngineInstance();

			// Upload the data
			std::generate(data.begin(), data.end(), SampleRand);
			buffer1_->UpdateSubresource(0, NUM_ELEMENTS * sizeof(uint32_t), &data[0]);

			// Sort the data
			// First sort the rows for the levels <= to the block size
			for (uint32_t level = 2; level <= BITONIC_BLOCK_SIZE; level = level * 2)
			{
				SetConstants(level, level, MATRIX_HEIGHT, MATRIX_WIDTH);

				// Sort the row data
				*(effect_->ParameterByName("Data")) = buffer1_uav_;
				re.Dispatch(*effect_, *technique_bitonic_, NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
			}

			// Then sort the rows and columns for the levels > than the block size
			// Transpose. Sort the Columns. Transpose. Sort the Rows.
			for (uint32_t level = (BITONIC_BLOCK_SIZE * 2); level <= NUM_ELEMENTS; level = level * 2)
			{
				SetConstants((level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT);

				// Transpose the data from buffer 1 into buffer 2
				*(effect_->ParameterByName("Data")) = buffer2_uav_;
				*(effect_->ParameterByName("Input")) = buffer1_srv_;
				re.Dispatch(*effect_, *technique_transpose_, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

				// Sort the transposed column data
				re.Dispatch(*effect_, *technique_bitonic_, NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);

				SetConstants(BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH);

				// Transpose the data from buffer 2 back into buffer 1
				*(effect_->ParameterByName("Data")) = buffer1_uav_;
				*(effect_->ParameterByName("Input")) = buffer2_srv_;
				re.Dispatch(*effect_, *technique_transpose_, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

				// Sort the row data
				re.Dispatch(*effect_, *technique_bitonic_, NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
			}

			buffer1_->CopyToBuffer(*result_cpu_buffer_);
			GraphicsBuffer::Mapper Mapper(*result_cpu_buffer_, BA_Read_Only);
			uint32_t const* pBuffer = reinterpret_cast<uint32_t const*>(Mapper.Pointer<uint32_t>());
			memcpy(&results[0], pBuffer, NUM_ELEMENTS * sizeof(uint32_t));
		}

	private:
		RenderEffectPtr effect_;

		RenderTechnique* technique_bitonic_ = nullptr;
		RenderTechnique* technique_transpose_ = nullptr;

		GraphicsBufferPtr result_cpu_buffer_;

		GraphicsBufferPtr buffer1_;
		UnorderedAccessViewPtr buffer1_uav_;
		ShaderResourceViewPtr buffer1_srv_;

		GraphicsBufferPtr buffer2_;
		UnorderedAccessViewPtr buffer2_uav_;
		ShaderResourceViewPtr buffer2_srv_;
	};

	enum
	{
		Exit
	};

	InputActionDefine actions[] = {InputActionDefine(Exit, KS_Escape)};
} // namespace


int SampleMain()
{
	BitonicSortApp app;
	app.Create();
	app.Run();

	return 0;
}

BitonicSortApp::BitonicSortApp() : App3DFramework("Bitonic Sort")
{
	ResLoader::Instance().AddPath("../../Tutorials/media/BitonicSort");
}

void BitonicSortApp::OnCreate()
{
	font_ = SyncLoadFont("gkai00mp.kfont");

	this->LookAt(float3(-1.2f, 2.2f, -1.2f), float3(0, 0.5f, 0));
	this->Proj(0.01f, 100);

	tb_controller_.AttachCamera(this->ActiveCamera());
	tb_controller_.Scalers(0.003f, 0.003f);

	InputEngine& inputEngine(Context::Instance().InputFactoryInstance().InputEngineInstance());
	InputActionMap actionMap;
	actionMap.AddActions(actions, actions + std::size(actions));

	action_handler_t input_handler = MakeSharedPtr<input_signal>();
	input_handler->Connect([this](InputEngine const& sender, InputAction const& action) { this->InputHandler(sender, action); });
	inputEngine.ActionMap(actionMap, input_handler);

	bitonic_ = MakeSharedPtr<BitonicSortObject>();
	Context::Instance().SceneManagerInstance().SceneRootNode().AddChild(bitonic_);

	UIManager::Instance().Load(ResLoader::Instance().Open("BitonicSort.uiml"));
}

void BitonicSortApp::OnResize(uint32_t width, uint32_t height)
{
	App3DFramework::OnResize(width, height);

	UIManager::Instance().SettleCtrls();
}

void BitonicSortApp::InputHandler(InputEngine const& /*sender*/, InputAction const& action)
{
	switch (action.first)
	{
	case Exit:
		this->Quit();
		break;
	}
}

void BitonicSortApp::DoUpdateOverlay()
{
	UIManager::Instance().Render();

	std::wostringstream stream;
	stream.precision(2);
	stream << std::fixed << this->FPS() << " FPS";

	font_->RenderText(0, 0, Color(1, 1, 0, 1), L"Bitonic Sort", 16);
	font_->RenderText(0, 18, Color(1, 1, 0, 1), stream.str().c_str(), 16);

	RenderFactory& rf = Context::Instance().RenderFactoryInstance();
	RenderEngine& re = rf.RenderEngineInstance();
	font_->RenderText(0, 36, Color(1, 1, 0, 1), re.ScreenFrameBuffer()->Description(), 16);
}

uint32_t BitonicSortApp::DoUpdate(uint32_t /*pass*/)
{
	KlayGE::RenderEngine& re = KlayGE::Context::Instance().RenderFactoryInstance().RenderEngineInstance();

	KlayGE::Color clear_clr(0.2f, 0.4f, 0.6f, 1);
	if (KlayGE::Context::Instance().Config().graphics_cfg.gamma)
	{
		clear_clr.r() = 0.029f;
		clear_clr.g() = 0.133f;
		clear_clr.b() = 0.325f;
	}

	checked_pointer_cast<BitonicSortObject>(bitonic_)->Update(this->AppTime(), this->FrameTime());

	re.CurFrameBuffer()->Clear(KlayGE::FrameBuffer::CBM_Color | KlayGE::FrameBuffer::CBM_Depth, clear_clr, 1.0f, 0);
	return KlayGE::App3DFramework::URV_NeedFlush | KlayGE::App3DFramework::URV_Finished;
}
