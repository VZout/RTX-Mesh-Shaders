float conv(in float[9] kernel, in float[9] data, in float denom, in float offset)
{
    float res = 0.0;
    for (int i=0; i<9; ++i)
    {
        res += kernel[i] * data[i];
    }
    return clamp(res/denom + offset, 0.0, 1.0);
}

struct ImageData
{
    float r[9];
    float g[9];
    float b[9];
    float avg[9];
};

ImageData GetNeighbours()
{
    ImageData image_data;

    int n = -1;
    for (int i=-1; i<2; ++i)
    {
        for(int j=-1; j<2; ++j)
        {
            n++;
            vec3 rgb = imageLoad(t_input, ivec2(gl_GlobalInvocationID.x + i, gl_GlobalInvocationID.y + j)).rgb;
            image_data.r[n] = rgb.r;
            image_data.g[n] = rgb.g;
            image_data.b[n] = rgb.b;
            image_data.avg[n] = (rgb.r + rgb.g + rgb.b) / 3.0;
        }
    }

    return image_data;
}

vec3 Sharpen(ImageData image_data)
{
    float[9] kernel;
    kernel[0] = -1.0; kernel[1] = -1.0; kernel[2] = -1.0;
    kernel[3] = -1.0; kernel[4] =  9.0; kernel[5] = -1.0;
    kernel[6] = -1.0; kernel[7] = -1.0; kernel[8] = -1.0;

    return vec3(
    conv(kernel, image_data.r, 1.0, 0.0),
    conv(kernel, image_data.g, 1.0, 0.0),
    conv(kernel, image_data.b, 1.0, 0.0));
}

vec3 EdgeDetect(ImageData image_data)
{
    float[9] kernel;
    kernel[0] = -1.0/8.0; kernel[1] = -1.0/8.0; kernel[2] = -1.0/8.0;
    kernel[3] = -1.0/8.0; kernel[4] =  1.0;     kernel[5] = -1.0/8.0;
    kernel[6] = -1.0/8.0; kernel[7] = -1.0/8.0; kernel[8] = -1.0/8.0;

    return vec3(conv(kernel, image_data.avg, 0.1, 0.0));
}

vec3 Embross(ImageData image_data)
{
    float[9] kernel;
    kernel[0] = -1.0; kernel[1] =  0.0; kernel[2] =  0.0;
    kernel[3] = 0.0; kernel[4] = -1.0; kernel[5] =  0.0;
    kernel[6] = 0.0; kernel[7] =  0.0; kernel[8] = 2.0;

    return vec3(conv(kernel, image_data.avg, 1.0, 0.50));
}
