#include "dx12_renderer.h"

#include "utils/com_error_handler.h"
#include "utils/window.h"

#include <filesystem>


void cg::renderer::dx12_renderer::init()
{
  // Default values
  rtv_descriptor_size = 0;
  view_port = CD3DX12_VIEWPORT(
    0.f,
    0.f,
    static_cast<float>(settings->width),
    static_cast<float>(settings->height)
  );
  scissor_rect = CD3DX12_RECT(
    0,
    0,
    static_cast<float>(settings->width),
    static_cast<float>(settings->height)
  );
  vertex_buffer_view = {};
  // TODO: verify matrices
  // world_view_projection =
  //   camera->get_dxm_view_matrix() *
  //   camera->get_dxm_projection_matrix();
  constant_buffer_data_begin = nullptr;
  frame_index = 0;

  for (size_t i = 0; i < frame_number; i++)
  {
    fence_values[i] = 0;
  }

  // Load model
  model = std::make_shared<cg::world::model>();
  model->load_obj(settings->model_path);

  // Prepare camera
  camera = std::make_shared<cg::world::camera>();
  camera = std::make_shared<cg::world::camera>();
  camera->set_width(static_cast<float>(settings->width));
  camera->set_height(static_cast<float>(settings->height));
  camera->set_position(float3{ settings->camera_position[0],
                               settings->camera_position[1],
                               settings->camera_position[2] });
  camera->set_theta(settings->camera_theta);
  camera->set_phi(settings->camera_phi);
  camera->set_angle_of_view(settings->camera_angle_of_view);
  camera->set_z_near(settings->camera_z_near);
  camera->set_z_far(settings->camera_z_far);

  load_pipeline();
  load_assets();
}

void cg::renderer::dx12_renderer::destroy()
{
  THROW_ERROR("Not implemented yet")
}

void cg::renderer::dx12_renderer::update()
{
  // THROW_ERROR("Not implemented yet")
}

void cg::renderer::dx12_renderer::render()
{
  // THROW_ERROR("Not implemented yet")
}

void cg::renderer::dx12_renderer::load_pipeline()
{
  UINT dxgi_factory_flags = 0;

#ifdef _DEBUG
  ComPtr<ID3D12Debug> debug_controller;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
  {
    debug_controller->EnableDebugLayer();
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  // Hardware adapter
  ComPtr<IDXGIFactory4> dxgi_factory;
  THROW_IF_FAILED(CreateDXGIFactory2(
    dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory))
  );

  ComPtr<IDXGIAdapter1> hardware_adapter;
  dxgi_factory->EnumAdapters1(1, &hardware_adapter);
#if _DEBUG
  DXGI_ADAPTER_DESC adapter_desc = {};
  hardware_adapter->GetDesc(&adapter_desc);
  OutputDebugString(adapter_desc.Description);
#endif

  // Create device
  THROW_IF_FAILED(
    D3D12CreateDevice(
      hardware_adapter.Get(),
      D3D_FEATURE_LEVEL_11_0,
      IID_PPV_ARGS(&device)
    )
  );

  // Create command queue
  D3D12_COMMAND_QUEUE_DESC queue_descriptor = {};
  queue_descriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_descriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  THROW_IF_FAILED(
    device->CreateCommandQueue(
      &queue_descriptor,
      IID_PPV_ARGS(&command_queue)
    )
  );

  // Create swap chain
  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
  swap_chain_desc.BufferCount = frame_number;
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.Width = settings->width;
  swap_chain_desc.Height = settings->height;
  swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.SampleDesc.Count = 1;

  ComPtr<IDXGISwapChain1> temp_swap_chain;
  THROW_IF_FAILED(
    dxgi_factory->CreateSwapChainForHwnd(
      command_queue.Get(),
      cg::utils::window::get_hwnd(),
      &swap_chain_desc,
      nullptr,
      nullptr,
      &temp_swap_chain
    )
  );

  THROW_IF_FAILED(
      dxgi_factory->MakeWindowAssociation(
      cg::utils::window::get_hwnd(),
      DXGI_MWA_NO_ALT_ENTER
    )
  );

  THROW_IF_FAILED(
    temp_swap_chain.As(&swap_chain)
  );

  frame_index = swap_chain->GetCurrentBackBufferIndex();

  // Create descriptor heap for render target
  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
  rtv_heap_desc.NumDescriptors = frame_number;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  THROW_IF_FAILED(
    device->CreateDescriptorHeap(
      &rtv_heap_desc,
      IID_PPV_ARGS(&rtv_heap)
    )
  );
  rtv_descriptor_size =
    device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // Create render target view for RTs
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
    rtv_heap->GetCPUDescriptorHandleForHeapStart()
  );

  for (UINT i = 0; i < frame_number; i++)
  {
    THROW_IF_FAILED(
      swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]))
    );

    device->CreateRenderTargetView(
      render_targets[i].Get(),
      nullptr,
      rtv_handle
    );

    rtv_handle.Offset(1, rtv_descriptor_size);

    // TODO: add name
  }

  D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
  cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  THROW_IF_FAILED(device->CreateDescriptorHeap(
    &cbv_heap_desc,
    IID_PPV_ARGS(&cbv_heap)
  ));
}

void cg::renderer::dx12_renderer::load_assets()
{
  // Create a root signature
  D3D12_FEATURE_DATA_ROOT_SIGNATURE rs_feature_data = {};
  rs_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

  if (
    FAILED(
      device->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE,
        &rs_feature_data,
        sizeof(rs_feature_data)
      )
    )
  )
  {
    rs_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  // Create descriptor tables
  CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
  CD3DX12_ROOT_PARAMETER1 root_parameters[1];
  ranges[0].Init(
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    1,
    0,
    0,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
  );
  root_parameters[0].InitAsDescriptorTable(
    1,
    &ranges[0],
    D3D12_SHADER_VISIBILITY_VERTEX
  );

  D3D12_ROOT_SIGNATURE_FLAGS rs_flags = 
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rs_descriptor;
  rs_descriptor.Init_1_1(
    _countof(root_parameters),
    root_parameters,
    0,
    nullptr,
    rs_flags
  );

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;

  HRESULT result = D3DX12SerializeVersionedRootSignature(
    &rs_descriptor,
    rs_feature_data.HighestVersion,
    &signature,
    &error
  );
  if (FAILED(result))
  {
    OutputDebugStringA((char*) error->GetBufferPointer());
    THROW_IF_FAILED(result);
  }

  // Compile shaders
  WCHAR buffer[MAX_PATH];
  GetModuleFileName(NULL, buffer, MAX_PATH);
  auto shader_path =
    std::filesystem::path(buffer).parent_path() / 
    "shaders.hlsl";

  ComPtr<ID3D10Blob> vertex_shader;
  ComPtr<ID3D10Blob> pixel_shader;

  UINT compile_flags = 0;
#ifdef _DEBUG
  compile_flags |=
      D3DCOMPILE_DEBUG
    | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  result = D3DCompileFromFile(
    shader_path.wstring().c_str(),
    nullptr,
    nullptr,
    "VSMain",
    "vs_5_0",
    compile_flags,
    0,
    &vertex_shader,
    &error
  );

  if (FAILED(result))
  {
    OutputDebugStringA((char*)error->GetBufferPointer());
    THROW_IF_FAILED(result);
  }

  result = D3DCompileFromFile(
    shader_path.wstring().c_str(),
    nullptr,
    nullptr,
    "PSMain",
    "ps_5_0",
    compile_flags,
    0,
    &pixel_shader,
    &error
  );

  if (FAILED(result))
  {
    OutputDebugStringA((char*)error->GetBufferPointer());
    THROW_IF_FAILED(result);
  }

  // Create and upload vertex buffer
  auto vertex_buffer_data = model->get_vertex_buffer();
  const UINT vertex_buffer_size = static_cast<UINT>(
    vertex_buffer_data->get_size_in_bytes()
  );

  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&vertex_buffer)
  );
  vertex_buffer->SetName(L"Vertex buffer");

  UINT8* vertex_data_begin;
  CD3DX12_RANGE read_range(0, 0);
  THROW_IF_FAILED(
    vertex_buffer->Map(
      0,
      &read_range,
      reinterpret_cast<void**>(&vertex_data_begin)
    )
  );
  memcpy(
    vertex_data_begin,
    vertex_buffer_data->get_data(),
    vertex_buffer_size
  );
  vertex_buffer->Unmap(0, nullptr);

  // Create VB descriptor
  vertex_buffer_view.BufferLocation =
    vertex_buffer->GetGPUVirtualAddress();
  vertex_buffer_view.SizeInBytes = vertex_buffer_size;
  vertex_buffer_view.StrideInBytes = sizeof(cg::vertex);

  // Create and upload constant buffer
  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(64*1024),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(&constant_buffer)
  );
  THROW_IF_FAILED(
    vertex_buffer->Map(
      0,
      &read_range,
      reinterpret_cast<void**>(&constant_buffer_data_begin)
    )
  );
  memcpy(
    constant_buffer_data_begin,
    &world_view_projection,
    sizeof(world_view_projection)
  );
  // Don't unmap constant buffer resource

  // Create CBV descriptor
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_descriptor = {};
  cbv_descriptor.BufferLocation =
    constant_buffer->GetGPUVirtualAddress();
  cbv_descriptor.SizeInBytes =
    (sizeof(world_view_projection) + 255) & ~255;

  device->CreateConstantBufferView(
    &cbv_descriptor,
    cbv_heap->GetCPUDescriptorHandleForHeapStart()
  );
}

void cg::renderer::dx12_renderer::populate_command_list()
{
  THROW_ERROR("Not implemented yet")
}


void cg::renderer::dx12_renderer::move_to_next_frame()
{
  THROW_ERROR("Not implemented yet")
}

void cg::renderer::dx12_renderer::wait_for_gpu()
{
  THROW_ERROR("Not implemented yet")
}
