#include <KlayGE/KlayGE.hpp>
#include <KFL/CXX17/iterator.hpp>
#include <KFL/Util.hpp>
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
#include <KlayGE/SceneNodeHelper.hpp>
#include <KlayGE/UI.hpp>
#include <KlayGE/Camera.hpp>

#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/InputFactory.hpp>

#include <vector>
#include <sstream>

#include "SampleCommon.hpp"
#include "SimpleBezier.hpp"
#include "MobiusStrip.hpp"

using namespace std;
using namespace KlayGE;

namespace
{
    enum
    {
        PT_Integer,
        PT_Even,
        PT_Odd,
    };

	class RenderBezier : public StaticMesh
	{
	public:
		explicit RenderBezier(std::wstring_view name) 
            : StaticMesh(name), partition_type_(PT_Integer), tess_factor_(8.0f), wireframe_(false)
		{
			effect_ = SyncLoadRenderEffect("SimpleBezier.fxml");
			UpdateTech();
		}

        void DoBuildMeshInfo(KlayGE::RenderModel const& model)
		{
			KFL_UNUSED(model);
		}

		void OnRenderBegin()
		{
			App3DFramework const & app = Context::Instance().AppInstance();
			Camera const & camera = app.ActiveCamera();

			*(effect_->ParameterByName("view_proj")) = camera.ViewProjMatrix();
			*(effect_->ParameterByName("camera_pos_world")) = camera.EyePos();
		}

        void TessellationFactor(float tess_factor)
        {
			tess_factor_ = tess_factor;
			*(effect_->ParameterByName("tessellation_factor")) = tess_factor_;
        }

        void PartitionType(uint32_t pt)
        {
			partition_type_ = pt;
			this->UpdateTech();
        }

        void Wireframe(bool wf)
        {
			wireframe_ = wf;
			this->UpdateTech();
        }

    private:
        void UpdateTech()
        {
			std::string tech_name;
            switch (partition_type_)
            {
			case PT_Integer:
				tech_name = "Integer";
				break;

            case PT_Even:
				tech_name = "Even";
				break;

            case PT_Odd:
				tech_name = "Odd";
				break;

            default:
				tech_name = "Integer";
				break;
            }

            if (wireframe_)
            {
				tech_name += "Wireframe";
            }

            technique_ = effect_->TechniqueByName(tech_name);
        }

    private:
		uint32_t partition_type_;
		float tess_factor_;
		bool wireframe_;
	};


	enum
	{
		Exit,
	};

	InputActionDefine actions[] =
	{
		InputActionDefine(Exit, KS_Escape),
	};
}


int SampleMain()
{
	SimpleBezier app;
	app.Create();
	app.Run();

	return 0;
}

SimpleBezier::SimpleBezier()
						: App3DFramework("SimpleBezier")
{
	ResLoader::Instance().AddPath("../../Tutorials/media/SimpleBezier");
}

void SimpleBezier::OnCreate()
{
	font_ = SyncLoadFont("gkai00mp.kfont");
	UIManager::Instance().Load(ResLoader::Instance().Open("SimpleBezier.uiml"));

    bezier_ = KlayGE::MakeSharedPtr<KlayGE::RenderModel>(L"model", KlayGE::SceneNode::SOA_Cullable);
	std::vector<KlayGE::StaticMeshPtr> mesh(1);

    mesh[0] = KlayGE::MakeSharedPtr<RenderBezier>(L"side_mesh");

	mesh[0]->NumLods(1);
	mesh[0]->AddVertexStream(0, g_MobiusStrip, sizeof(g_MobiusStrip), 
        KlayGE::VertexElement(KlayGE::VEU_Position, 0, KlayGE::EF_BGR32F), KlayGE::EAH_GPU_Read);
	mesh[0]->GetRenderLayout().TopologyType(KlayGE::RenderLayout::TT_16_Ctrl_Pt_PatchList);
	mesh[0]->PosBound(KlayGE::AABBox(KlayGE::float3(0, 0, 0), KlayGE::float3(20, 20, 20)));

    mesh[0]->BuildMeshInfo(*bezier_);
    bezier_->AssignMeshes(mesh.begin(), mesh.end());
	bezier_->BuildModelInfo();

    bezier_->RootNode()->AddRenderable(mesh[0]);
	Context::Instance().SceneManagerInstance().SceneRootNode().AddChild(bezier_->RootNode());

	this->LookAt(float3(0, 0, -10), float3(0, 0, 0));
	this->Proj(0.1f, 20.0f);

	tb_controller_.AttachCamera(this->ActiveCamera());
	tb_controller_.Scalers(0.05f, 0.1f);

	InputEngine& inputEngine(Context::Instance().InputFactoryInstance().InputEngineInstance());
	InputActionMap actionMap;
	actionMap.AddActions(actions, actions + std::size(actions));

	action_handler_t input_handler = MakeSharedPtr<input_signal>();
	input_handler->connect(
		[this](InputEngine const & sender, InputAction const & action)
		{
			this->InputHandler(sender, action);
		}
    );
	inputEngine.ActionMap(actionMap, input_handler);
}

void SimpleBezier::OnResize(uint32_t width, uint32_t height)
{
	App3DFramework::OnResize(width, height);

	UIManager::Instance().SettleCtrls();
}

void SimpleBezier::InputHandler(InputEngine const& /*sender*/, InputAction const& action)
{
	switch (action.first)
	{
	case Exit:
		this->Quit();
		break;
	}
}

void SimpleBezier::DoUpdateOverlay()
{
	UIManager::Instance().Render();

	RenderEngine& renderEngine(Context::Instance().RenderFactoryInstance().RenderEngineInstance());

	std::wostringstream stream;
	stream.precision(2);
	stream << std::fixed << this->FPS() << " FPS";

	font_->RenderText(0, 0, Color(1, 1, 0, 1), L"Simple Bezier", 16);
	font_->RenderText(0, 18, Color(1, 1, 0, 1), stream.str(), 16);

	SceneManager& sceneMgr(Context::Instance().SceneManagerInstance());
	stream.str(L"");
	stream << sceneMgr.NumRenderablesRendered() << " Renderables "
		<< sceneMgr.NumPrimitivesRendered() << " Primitives "
		<< sceneMgr.NumVerticesRendered() << " Vertices";
	font_->RenderText(0, 36, Color(1, 1, 1, 1), stream.str(), 16);
	font_->RenderText(0, 54, Color(1, 1, 0, 1), renderEngine.Name(), 16);
}

void SimpleBezier::TessFactorChangedHandler(KlayGE::UISlider const& sender)
{
	tess_factor_ = sender.GetValue() / 10.0f;
    bezier_->ForEachMesh([this](Renderable& renderable)
    {
        checked_cast<RenderBezier*>(&renderable)->TessellationFactor(tess_factor_);
    });

	std::wostringstream stream;
	stream << L"Tess Factor: " << tess_factor_;
	dialog_->Control<UIStatic>(id_tessellation_factor_static_)->SetText(stream.str());
}

void SimpleBezier::PartitionTypeChangedHandler(KlayGE::UIComboBox const& sender)
{
	int const index = sender.GetSelectedIndex();
	bezier_->ForEachMesh([index](Renderable& renderable)
    {
        checked_cast<RenderBezier*>(&renderable)->PartitionType(index);
    });
}

void SimpleBezier::WireframeHandler(KlayGE::UICheckBox const& sender)
{
	bool const wf = sender.GetChecked();
	bezier_->ForEachMesh([wf](Renderable& renderable)
    {
        checked_cast<RenderBezier*>(&renderable)->Wireframe(wf);
    });
}

uint32_t SimpleBezier::DoUpdate(uint32_t /*pass*/)
{
	RenderEngine& renderEngine(Context::Instance().RenderFactoryInstance().RenderEngineInstance());

    dialog_ = UIManager::Instance().GetDialog("SimpleBezier");
	dialog_->SetVisible(true);

    id_tessellation_factor_static_ = dialog_->IDFromName("TessFactorStatic");
	id_tessellation_factor_slider_ = dialog_->IDFromName("TessFactorSlider");
    id_partition_type_static_ = dialog_->IDFromName("PartitionTypeStatic");
	id_partition_type_combo_ = dialog_->IDFromName("PartitionTypeCombo");
	id_wireframe_ = dialog_->IDFromName("Wireframe");

    dialog_->Control<UISlider>(id_tessellation_factor_slider_)->SetValue(static_cast<int>(tess_factor_ * 10));
	dialog_->Control<UISlider>(id_tessellation_factor_slider_)->OnValueChangedEvent().connect(
        [this](UISlider const& sender) {
	    	this->TessFactorChangedHandler(sender);
	    }
    );
	this->TessFactorChangedHandler(*dialog_->Control<UISlider>(id_tessellation_factor_slider_));

	dialog_->Control<UIComboBox>(id_partition_type_combo_)->OnSelectionChangedEvent().connect(
        [this](UIComboBox const &sender)
        {
		    this->PartitionTypeChangedHandler(sender);
        }
    );
	this->PartitionTypeChangedHandler(*dialog_->Control<UIComboBox>(id_partition_type_combo_));

    dialog_->Control<UICheckBox>(id_wireframe_)->OnChangedEvent().connect(
        [this](UICheckBox const& sender) {
	        this->WireframeHandler(sender);
	    }
    );
	this->WireframeHandler(*dialog_->Control<UICheckBox>(id_wireframe_));

	Color clear_clr(0.2f, 0.4f, 0.6f, 1);
	if (Context::Instance().Config().graphics_cfg.gamma)
	{
		clear_clr.r() = 0.029f;
		clear_clr.g() = 0.133f;
		clear_clr.b() = 0.325f;
	}

	renderEngine.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color | FrameBuffer::CBM_Depth, clear_clr, 1.0f, 0);
	return App3DFramework::URV_NeedFlush | App3DFramework::URV_Finished;
}

