// SceneObjectHelper.hpp
// KlayGE һЩ���õĳ������� ͷ�ļ�
// Ver 3.12.0
// ��Ȩ����(C) ������, 2005-2011
// Homepage: http://www.klayge.org
//
// 3.10.0
// SceneObjectSkyBox��SceneObjectHDRSkyBox������Technique() (2010.1.4)
//
// 3.9.0
// ������SceneObjectHDRSkyBox (2009.5.4)
//
// 3.1.0
// ���ν��� (2005.10.31)
//
// �޸ļ�¼
//////////////////////////////////////////////////////////////////////////////////

#ifndef _SCENEOBJECTHELPER_HPP
#define _SCENEOBJECTHELPER_HPP

#pragma once

#include <KlayGE/PreDeclare.hpp>
#include <KlayGE/SceneNode.hpp>
#include <KFL/AABBox.hpp>
#include <KlayGE/Mesh.hpp>

namespace KlayGE
{
	KLAYGE_CORE_API RenderModelPtr LoadLightSourceProxyModel(LightSourcePtr const& light,
		std::function<StaticMeshPtr(std::wstring_view)> CreateMeshFactoryFunc = CreateMeshFactory<RenderableLightSourceProxy>);

	KLAYGE_CORE_API RenderModelPtr LoadCameraProxyModel(CameraPtr const& camera,
		std::function<StaticMeshPtr(std::wstring_view)> CreateMeshFactoryFunc = CreateMeshFactory<RenderableCameraProxy>);
}

#endif		// _RENDERABLEHELPER_HPP
