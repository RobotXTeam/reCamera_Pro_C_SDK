#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <rknn/rknn_api.h>

int main() {
    std::ifstream ifs("/oem/usr/share/model/rknn/yolox_s.rknn", std::ios::binary);
    if (!ifs.good()) {
        printf("Cannot open model file.\n");
        return 1;
    }
    ifs.seekg(0, std::ios::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> model_data(size);
    ifs.read(model_data.data(), size);
    
    rknn_context ctx;
    int ret = rknn_init(&ctx, model_data.data(), size, 0, NULL);
    if (ret != RKNN_SUCC) {
        printf("rknn_init failed: %d\n", ret);
        return 1;
    }
    
    rknn_input_output_num io_num;
    rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    printf("Inputs: %d, Outputs: %d\n", io_num.n_input, io_num.n_output);
    
    std::vector<rknn_tensor_attr> output_attrs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr));
    }
    
    std::vector<uint8_t> dummy_img(640 * 640 * 3, 128);
    rknn_input inputs[1];
    inputs[0].index = 0;
    inputs[0].buf = dummy_img.data();
    inputs[0].size = dummy_img.size();
    inputs[0].pass_through = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    
    ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    if (ret != RKNN_SUCC) {
        printf("rknn_inputs_set failed\n");
        return 1;
    }
    
    ret = rknn_run(ctx, NULL);
    if (ret != RKNN_SUCC) {
        printf("rknn_run failed\n");
        return 1;
    }
    
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; i++) {
        outputs[i].want_float = 1;
        outputs[i].is_prealloc = 0;
    }
    
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs.data(), NULL);
    if (ret != RKNN_SUCC) {
        printf("rknn_outputs_get failed\n");
        return 1;
    }
    
    printf("Output 0 size: %d, buf: %p\n", outputs[0].size, outputs[0].buf);
    if (outputs[0].buf) {
        float* f = (float*)outputs[0].buf;
        printf("Val: %f %f\n", f[0], f[1]);
    }
    
    rknn_outputs_release(ctx, io_num.n_output, outputs.data());
    rknn_destroy(ctx);
    return 0;
}
