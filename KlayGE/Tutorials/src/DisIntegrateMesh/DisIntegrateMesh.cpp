#include <KlayGE/KlayGE.hpp>
#include <KFL/CXX17/iterator.hpp>
#include <KFL/Util.hpp>
#include <KFL/Half.hpp>
#include <KFL/Math.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/Renderable.hpp>
#include <KlayGE/RenderableHelper.hpp>
#include <KlayGE/RenderEngine.hpp>
#include <KlayGE/RenderEffect.hpp>
#include <KlayGE/FrameBuffer.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/RenderSettings.hpp>
#include <KlayGE/Mesh.hpp>
#include <KlayGE/RenderableHelper.hpp>
#include <KlayGE/SceneNodeHelper.hpp>
#include <KlayGE/ElementFormat.hpp>
#include <KlayGE/UI.hpp>
#include <KlayGE/Camera.hpp>

#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/InputFactory.hpp>

#include <vector>
#include <sstream>
#include <fstream>
#include <random>

#include "SampleCommon.hpp"
#include "DisIntegrateMesh.hpp"

using namespace std;
using namespace KlayGE;

namespace
{
	int const NUM_PARTICLE = 1024 * 1024;
	int const NUM_EMITTER = 16;

	class RenderParticles : public Renderable
	{
	public:
		explicit RenderParticles()
			: Renderable(L"RenderParticles")
		{
			RenderFactory& rf = Context::Instance().RenderFactoryInstance();

			rls_[0] = rf.MakeRenderLayout();

			effect_ = SyncLoadRenderEffect("DisIntegrateMesh.fxml");
			*(effect_->ParameterByName("particle_tex")) = ASyncLoadTexture("particle.dds", EAH_GPU_Read | EAH_Immutable);

			rls_[0]->TopologyType(RenderLayout::TT_PointList);

			technique_ = effect_->TechniqueByName("ParticlesWithGSSO");

			*(effect_->ParameterByName("point_radius")) = 0.025f;
			*(effect_->ParameterByName("init_pos_life")) = float4(0, 0, 0, 8);
		}

		void SceneTexture(TexturePtr const & tex)
		{
			*(effect_->ParameterByName("scene_tex")) = tex;
		}

		void OnRenderBegin()
		{
			App3DFramework const & app = Context::Instance().AppInstance();
			Camera const & camera = app.ActiveCamera();

			*(effect_->ParameterByName("View")) = camera.ViewMatrix();
			*(effect_->ParameterByName("Proj")) = camera.ProjMatrix();
			*(effect_->ParameterByName("inv_view")) = camera.InverseViewMatrix();
			*(effect_->ParameterByName("inv_proj")) = camera.InverseProjMatrix();

			*(effect_->ParameterByName("far_plane")) = app.ActiveCamera().FarPlane();
		}

		void PosVB(GraphicsBufferPtr const & particle_pos_vb)
		{
			rls_[0]->BindVertexStream(particle_pos_vb, VertexElement(VEU_Position, 0, EF_ABGR32F));
		}
	};

	class DisIntegrateMesh : public Renderable
	{
	public:
		DisIntegrateMesh(int max_num_particles, int max_num_emitter)
			: Renderable(L"DisIntegrateMesh"),
			max_num_particles_(max_num_particles), max_num_emitter_(max_num_emitter),
			random_dis_(0, +500)
		{
			RenderFactory& rf = Context::Instance().RenderFactoryInstance();

			effect_ = SyncLoadRenderEffect("DisIntegrateMesh.fxml");
			technique_append_ = effect_->TechniqueByName("AppendCS");
			technique_update_ = effect_->TechniqueByName("UpdateCS");

			particles_.reserve(max_num_particles_);
			for (int i = 0; i < max_num_particles_; ++i)
			{
				particles_.push_back(float4(0, 0, 0, -1));
			}

			uint32_t access_hint = EAH_GPU_Read | EAH_GPU_Write | EAH_GPU_Unordered;

			particle_pos_vb_ = rf.MakeVertexBuffer(BU_Dynamic, access_hint, max_num_particles_ * sizeof(float4), &particles_[0], sizeof(float4));
			particle_pos_uav_ = rf.MakeBufferUav(particle_pos_vb_, EF_ABGR32F);

			pos_cpu_buffer_ = rf.MakeVertexBuffer(BU_Dynamic, EAH_CPU_Read, particle_pos_vb_->Size(), nullptr);

			particle_emit_vb_ = rf.MakeVertexBuffer(BU_Dynamic, 
				access_hint | EAH_GPU_Structured | EAH_Counter, 
				max_num_particles_ * sizeof(float4), &particles_[0], sizeof(float4));
			particle_emit_uav_ = rf.MakeBufferUav(particle_emit_vb_, EF_ABGR32F);

			*(effect_->ParameterByName("particle_pos_rw_buff")) = particle_pos_uav_;
			*(effect_->ParameterByName("particle_emit_buff")) = particle_emit_uav_;

			emit_pos_.reserve(max_num_emitter_);
			for (int i = 0; i < max_num_emitter_; ++i)
			{
				emit_pos_.push_back(float4(i * 0.1f, i * 0.1f, 0, -1));
			}
		}

		void AutoEmitWithCpuBuffer(float elapsed_time)
		{
			static float accumulate_time = 0;

			accumulate_time += elapsed_time;
			if (accumulate_time < 0.05f)
				return;

			accumulate_time -= 0.05f;

			particle_pos_vb_->CopyToBuffer(*pos_cpu_buffer_);
			GraphicsBuffer::Mapper Mapper(*pos_cpu_buffer_, BA_Read_Only);
			float4 const* pBuffer = reinterpret_cast<float4 const*>(Mapper.Pointer<float4>());
			
			for (int i = 0, j = 0; i < max_num_particles_; ++i)
			{
				if (pBuffer[i].w() <= 0 && j < max_num_emitter_)
				{
					particles_[i] = float4(emit_pos_[j].x(), emit_pos_[j].y(), emit_pos_[j].z(), 8.0f);//RandomGen());
					++j;
				}
				else
				{
					particles_[i] = pBuffer[i];
				}
			}

			particle_pos_vb_->UpdateSubresource(0, max_num_particles_ * sizeof(float4), &particles_[0]);
		}

		void Update(float /*app_time*/, float elapsed_time)
		{
			RenderEngine& re = Context::Instance().RenderFactoryInstance().RenderEngineInstance();

			//AutoEmitWithCpuBuffer(elapsed_time);

			re.BindFrameBuffer(re.DefaultFrameBuffer());
			re.DefaultFrameBuffer()->Discard(FrameBuffer::CBM_Color);

			*(effect_->ParameterByName("elapse_time")) = elapsed_time;
			*(effect_->ParameterByName("particle_velocity")) = -2.0f;

			this->OnRenderBegin();
			technique_ = technique_append_;
			re.Dispatch(*effect_, *technique_, 1, 1, 1);

			technique_ = technique_update_;
			re.Dispatch(*effect_, *technique_, (max_num_particles_ + 255) / 256, 1, 1);
			this->OnRenderEnd();

			particle_emit_vb_->UpdateSubresource(0, max_num_particles_ * sizeof(float4), &particles_[0]);
		}

		GraphicsBufferPtr PosVB() const
		{
			return particle_pos_vb_;
		}

		ShaderResourceViewPtr PosSrv() const
		{
			return particle_pos_srv_;
		}

	private:
		float RandomGen()
		{
			return MathLib::clamp(random_dis_(gen_) * 0.1f, 1.0f, 32.0f);
		}

	private:
		int max_num_particles_;
		int max_num_emitter_;

		RenderTechnique* technique_update_ = nullptr;
		RenderTechnique* technique_append_ = nullptr;

		GraphicsBufferPtr particle_pos_vb_;
		UnorderedAccessViewPtr particle_pos_uav_;
		ShaderResourceViewPtr particle_pos_srv_;

		GraphicsBufferPtr particle_emit_vb_;
		UnorderedAccessViewPtr particle_emit_uav_;

		GraphicsBufferPtr pos_cpu_buffer_;

		std::vector<float4> emit_pos_;
		std::vector<float4> particles_;

		ranlux24_base gen_;
		uniform_int_distribution<> random_dis_;
	};

	std::shared_ptr<DisIntegrateMesh> gpu_ps;

	enum
	{
		Exit
	};

	InputActionDefine actions[] =
	{
		InputActionDefine(Exit, KS_Escape)
	};
}


int SampleMain()
{
	DisIntegrateMeshApp app;
	app.Create();
	app.Run();

	gpu_ps.reset();

	return 0;
}

DisIntegrateMeshApp::DisIntegrateMeshApp()
	: App3DFramework("DisIntegrateMesh System")
{
	ResLoader::Instance().AddPath("../../Tutorials/media/DisIntegrateMesh");
}

void DisIntegrateMeshApp::OnCreate()
{
	RenderFactory& rf = Context::Instance().RenderFactoryInstance();
	RenderEngine& re = rf.RenderEngineInstance();

	font_ = SyncLoadFont("gkai00mp.kfont");

	this->LookAt(float3(-1.2f, 2.2f, -1.2f), float3(0, 0.5f, 0));
	this->Proj(0.01f, 100);

	tb_controller_.AttachCamera(this->ActiveCamera());
	tb_controller_.Scalers(0.003f, 0.003f);

	InputEngine& inputEngine(Context::Instance().InputFactoryInstance().InputEngineInstance());
	InputActionMap actionMap;
	actionMap.AddActions(actions, actions + std::size(actions));

	action_handler_t input_handler = MakeSharedPtr<input_signal>();
	input_handler->Connect(
		[this](InputEngine const & sender, InputAction const & action)
	{
		this->InputHandler(sender, action);
	});
	inputEngine.ActionMap(actionMap, input_handler);

	particles_renderable_ = MakeSharedPtr<RenderParticles>();
	particles_ = MakeSharedPtr<SceneNode>(MakeSharedPtr<RenderableComponent>(particles_renderable_), SceneNode::SOA_Moveable);
	Context::Instance().SceneManagerInstance().SceneRootNode().AddChild(particles_);

	gpu_ps = MakeSharedPtr<DisIntegrateMesh>(NUM_PARTICLE, NUM_EMITTER);

	FrameBufferPtr const & screen_buffer = re.CurFrameBuffer();

	scene_buffer_ = rf.MakeFrameBuffer();
	scene_buffer_->GetViewport()->camera = screen_buffer->GetViewport()->camera;
	fog_buffer_ = rf.MakeFrameBuffer();
	fog_buffer_->GetViewport()->camera = screen_buffer->GetViewport()->camera;

	blend_pp_ = SyncLoadPostProcess("Blend.ppml", "blend");

	checked_pointer_cast<RenderParticles>(particles_renderable_)->PosVB(gpu_ps->PosVB());

	UIManager::Instance().Load(ResLoader::Instance().Open("DisIntegrateMesh.uiml"));
}

void DisIntegrateMeshApp::OnResize(uint32_t width, uint32_t height)
{
	App3DFramework::OnResize(width, height);

	RenderFactory& rf = Context::Instance().RenderFactoryInstance();

	auto ds_view = rf.Make2DDsv(width, height, EF_D16, 1, 0);

	scene_tex_ = rf.MakeTexture2D(width, height, 1, 1, EF_ABGR16F, 1, 0, EAH_GPU_Read | EAH_GPU_Write);
	scene_buffer_->Attach(FrameBuffer::Attachment::Color0, rf.Make2DRtv(scene_tex_, 0, 1, 0));
	scene_buffer_->Attach(ds_view);

	fog_tex_ = rf.MakeTexture2D(width, height, 1, 1, EF_ABGR16F, 1, 0, EAH_GPU_Read | EAH_GPU_Write);
	fog_buffer_->Attach(FrameBuffer::Attachment::Color0, rf.Make2DRtv(fog_tex_, 0, 1, 0));
	fog_buffer_->Attach(ds_view);

	checked_pointer_cast<RenderParticles>(particles_renderable_)->SceneTexture(scene_tex_);

	blend_pp_->InputPin(0, scene_tex_);

	UIManager::Instance().SettleCtrls();
}

void DisIntegrateMeshApp::InputHandler(InputEngine const & /*sender*/, InputAction const & action)
{
	switch (action.first)
	{
	case Exit:
		this->Quit();
		break;
	}
}

void DisIntegrateMeshApp::DoUpdateOverlay()
{
	UIManager::Instance().Render();

	std::wostringstream stream;
	stream.precision(2);
	stream << std::fixed << this->FPS() << " FPS";

	font_->RenderText(0, 0, Color(1, 1, 0, 1), L"DisIntegrateMesh System", 16);
	font_->RenderText(0, 18, Color(1, 1, 0, 1), stream.str().c_str(), 16);

	RenderFactory& rf = Context::Instance().RenderFactoryInstance();
	RenderEngine& re = rf.RenderEngineInstance();
	font_->RenderText(0, 36, Color(1, 1, 0, 1), re.ScreenFrameBuffer()->Description(), 16);
}

uint32_t DisIntegrateMeshApp::DoUpdate(uint32_t pass)
{
	RenderFactory& rf = Context::Instance().RenderFactoryInstance();
	RenderEngine& re = rf.RenderEngineInstance();

	switch (pass)
	{
	case 0:
	{
		re.BindFrameBuffer(scene_buffer_);
		Color clear_clr(0.2f, 0.4f, 0.6f, 1);
		if (Context::Instance().Config().graphics_cfg.gamma)
		{
			clear_clr.r() = 0.029f;
			clear_clr.g() = 0.133f;
			clear_clr.b() = 0.325f;
		}
		re.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color | FrameBuffer::CBM_Depth, clear_clr, 1.0f, 0);
	}
	return App3DFramework::URV_NeedFlush;

	case 1:
	{
		particles_->Visible(true);

		gpu_ps->Update(this->AppTime(), this->FrameTime());

		re.BindFrameBuffer(fog_buffer_);
		re.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color | FrameBuffer::CBM_Depth, Color(0, 0, 0, 0), 1.0f, 0);

		return App3DFramework::URV_NeedFlush;
	}

	default:
	{
		particles_->Visible(false);

		blend_pp_->InputPin(1, fog_tex_);

		re.BindFrameBuffer(FrameBufferPtr());
		re.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color | FrameBuffer::CBM_Depth, Color(0, 0, 0, 1), 1.0f, 0);

		blend_pp_->Apply();

		return App3DFramework::URV_NeedFlush | App3DFramework::URV_Finished;
	}
	}
}
