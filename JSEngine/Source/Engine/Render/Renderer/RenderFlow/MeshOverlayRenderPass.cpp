#include "MeshOverlayRenderPass.h"

#include "Core/Logging/Stats.h"
#include "Core/ResourceManager.h"
#include "Render/Common/ShaderBindingSlots.h"
#include "Render/Resource/RenderResources.h"
#include "Render/Resource/ShaderPaths.h"
#include "Render/Resource/VertexFactoryTypes.h"
#include "Render/Scene/RenderBus.h"

namespace
{
    FShaderProgram* GetBoneWeightHeatmapProgram()
    {
        const FVertexFactoryDesc& VertexFactoryDesc =
            FVertexFactoryRegistry::Get(EVertexFactoryType::SkeletalMeshOverlay);

        FShaderStageKey VSKey;
        VSKey.FilePath = FShaderPaths::EditorBoneWeightHeatmap;
        VSKey.EntryPoint = "VS";
        VSKey.Target = "vs_5_0";

        FShaderStageKey PSKey;
        PSKey.FilePath = FShaderPaths::EditorBoneWeightHeatmap;
        PSKey.EntryPoint = "PS";
        PSKey.Target = "ps_5_0";

        return FResourceManager::Get().GetOrCreateShaderProgram(
            VSKey,
            PSKey,
            nullptr,
            nullptr,
            &VertexFactoryDesc.VertexLayout);
    }
}

bool FMeshOverlayRenderPass::Initialize()
{
    return true;
}

bool FMeshOverlayRenderPass::Release()
{
    return true;
}

bool FMeshOverlayRenderPass::Begin(const FRenderPassContext* Context)
{
    ID3D11RenderTargetView* RTV = PrevPassRTV;
    ID3D11DepthStencilView* DSV = Context->RenderTargets->DepthStencilView;
    Context->DeviceContext->OMSetRenderTargets(1, &RTV, DSV);
    Context->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11DepthStencilState* DepthState =
        FResourceManager::Get().GetOrCreateDepthStencilState(EDepthStencilType::DepthReadOnly);
    ID3D11BlendState* BlendState =
        FResourceManager::Get().GetOrCreateBlendState(EBlendType::AlphaBlend);
    ID3D11RasterizerState* RasterizerState =
        FResourceManager::Get().GetOrCreateRasterizerState(ERasterizerType::SolidNoCull);

    Context->DeviceContext->OMSetDepthStencilState(DepthState, 0);
    Context->DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
    Context->DeviceContext->RSSetState(RasterizerState);

    OutSRV = PrevPassSRV;
    OutRTV = PrevPassRTV;
    return true;
}

bool FMeshOverlayRenderPass::DrawCommand(const FRenderPassContext* Context)
{
    SCOPE_STAT("GPU.SkeletalDraw.MeshOverlay");
    const TArray<FRenderCommand>& Commands = Context->RenderBus->GetCommands(ERenderPass::MeshOverlay);
    if (Commands.empty())
    {
        return true;
    }

    FShaderProgram* Program = GetBoneWeightHeatmapProgram();
    if (!Program)
    {
        return true;
    }

    for (const FRenderCommand& Cmd : Commands)
    {
        if (Cmd.MeshOverlay.Mode != EMeshOverlayMode::BoneWeightHeatmap)
        {
            continue;
        }

        if (Cmd.MeshBuffer == nullptr || !Cmd.MeshBuffer->IsValid())
        {
            continue;
        }

        ID3D11Buffer* VertexBuffer = Cmd.MeshBuffer->GetVertexBuffer().GetBuffer();
        if (VertexBuffer == nullptr)
        {
            continue;
        }

        const uint32 VertexCount = Cmd.MeshBuffer->GetVertexBuffer().GetVertexCount();
        const uint32 Stride = Cmd.MeshBuffer->GetVertexBuffer().GetStride();
        if (VertexCount == 0 || Stride == 0)
        {
            continue;
        }

        Program->Bind(Context->DeviceContext);
        BindVertexFactoryResources(Context->DeviceContext, Cmd.VertexFactoryType, Cmd);

        Context->RenderResources->PerObjectConstantBuffer.Update(
            Context->DeviceContext,
            &Cmd.PerObjectConstants,
            sizeof(FPerObjectConstants));
        ID3D11Buffer* PerObjectCB = Context->RenderResources->PerObjectConstantBuffer.GetBuffer();
        Context->DeviceContext->VSSetConstantBuffers(ShaderBindingSlots::PerObjectCB, 1, &PerObjectCB);
        Context->DeviceContext->PSSetConstantBuffers(ShaderBindingSlots::PerObjectCB, 1, &PerObjectCB);

        Context->RenderResources->MeshOverlayConstantBuffer.Update(
            Context->DeviceContext,
            &Cmd.MeshOverlay,
            sizeof(FMeshOverlayConstants));
        ID3D11Buffer* OverlayCB = Context->RenderResources->MeshOverlayConstantBuffer.GetBuffer();
        Context->DeviceContext->VSSetConstantBuffers(12, 1, &OverlayCB);
        Context->DeviceContext->PSSetConstantBuffers(12, 1, &OverlayCB);

        uint32 Offset = 0;
        Context->DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

        ID3D11Buffer* IndexBuffer = Cmd.MeshBuffer->GetIndexBuffer().GetBuffer();
        if (IndexBuffer != nullptr)
        {
            Context->DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Context->DeviceContext->DrawIndexed(Cmd.SectionIndexCount, Cmd.SectionIndexStart, 0);
        }
        else
        {
            Context->DeviceContext->Draw(VertexCount, 0);
        }
    }

    return true;
}

bool FMeshOverlayRenderPass::End(const FRenderPassContext* Context)
{
    ID3D11ShaderResourceView* NullSRV = nullptr;
    Context->DeviceContext->VSSetShaderResources(ShaderBindingSlots::BoneMatricesSRV, 1, &NullSRV);
    return true;
}
