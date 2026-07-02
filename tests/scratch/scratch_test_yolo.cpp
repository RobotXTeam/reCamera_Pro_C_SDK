#include <rv1126b/npu.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace rv1126b;

int main() {
    NPU npu;
    npu.init();

    auto load_res = npu.load_model("/oem/usr/share/model/rknn/yolox_s.rknn");
    ModelHandle handle = *load_res;
    auto info = *npu.get_model_info(handle);

    std::vector<uint8_t> dummy_img(640 * 640 * 3, 0);
    npu::TensorBuffer input;
    input.index = 0;
    input.data = dummy_img.data();
    input.size = dummy_img.size();
    input.dtype = npu::DataType::Uint8;

    std::vector<npu::TensorBuffer> outputs(info.output_count);
    for (uint32_t i = 0; i < info.output_count; i++) {
        outputs[i].index = i;
        outputs[i].dtype = npu::DataType::Float32;
        outputs[i].size = info.outputs[i].element_count() * sizeof(float);
        outputs[i].data = malloc(outputs[i].size);
    }

    auto infer_res = npu.infer(handle, {input}, outputs);
    if (!infer_res.ok()) {
        printf("Infer failed!\n");
    } else {
        printf("Infer success! outputs[0].data = %p, val = %f\n", outputs[0].data, ((float*)outputs[0].data)[0]);
    }

    npu.release_outputs(handle, outputs); // Wait, if I malloc'd, do I free? release_outputs might crash if it tries to free user memory! Let's see.
    for (uint32_t i = 0; i < info.output_count; i++) {
        free(outputs[i].data);
    }

    npu.unload_model(handle);
    npu.deinit();
    return 0;
}
