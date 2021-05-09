#include <string>
#include <cmath>

#include "avisynth.h"

#ifndef M_PI // GCC seems to have it
constexpr double M_PI = 3.14159265358979323846;
#endif

template <typename T>
static inline void memset16(T* ptr, int value, size_t num)
{
    while (num-- > 0)
        *ptr++ = value;
}

template <typename T>
class vsCnr2 : public GenericVideoFilter
{
    bool scenechroma;

    PVideoFrame prev;
    int last_frame = 0;
    T* table_y;
    T* table_u;
    T* table_v;
    T* curp_y;
    T* prevp_y;
    int64_t diff_max;
    bool v8;
    int subsw, subsh;
    int depth;
    int range_max;
    int64_t shift;
    int shift1, shift2;

    void downSampleLuma(T* dstp, PVideoFrame& src);

public:
    vsCnr2(PClip _child, std::string mode, double scdthr, int ln, int lm, int un, int um, int vn, int vm, bool _scenechroma, IScriptEnvironment* env);

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_SERIALIZED : 0;
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
    ~vsCnr2();
};

template <typename T>
void vsCnr2<T>::downSampleLuma(T* dstp, PVideoFrame& src)
{
    const int src_stride = src->GetPitch() / sizeof(T);
    const int dst_width = src->GetRowSize(PLANAR_U) / sizeof(T);
    const int dst_height = src->GetHeight(PLANAR_U);
    const T* srcp = reinterpret_cast<const T*>(src->GetReadPtr());

    for (int y = 0; y < dst_height; ++y)
    {
        const T* srcpn = srcp + (static_cast<int64_t>(src_stride) * subsh);

        for (int x = 0; x < dst_width; ++x)
        {
            const int temp = x << subsw;
            dstp[x] = (srcp[temp] + srcp[temp + 1] + srcpn[temp] + srcpn[temp + 1] + 2) >> 2;
        }

        srcp += static_cast<int64_t>(src_stride) << subsh;
        dstp += dst_width;
    }
}

template <typename T>
vsCnr2<T>::vsCnr2(PClip _child, std::string mode, double scdthr, int ln, int lm, int un, int um, int vn, int vm, bool _scenechroma, IScriptEnvironment* env)
    : GenericVideoFilter(_child), scenechroma(_scenechroma)
{
    subsw = vi.GetPlaneWidthSubsampling(PLANAR_U);
    subsh = vi.GetPlaneHeightSubsampling(PLANAR_U);
    depth = vi.BitsPerComponent();

    if (vi.IsRGB() || depth == 32 || !vi.IsPlanar() || vi.NumComponents() < 3)
        env->ThrowError("vsCnr2: clip must be in YUV 8..16-bit planar format and must have at least three planes.");
    if (subsh > 1 || subsw > 1)
        env->ThrowError("vsCnr2: clip must have chroma subsampling 420, 422, 440 or 444.");
    if (mode.size() < 3)
        env->ThrowError("vsCnr2: mode must have at least three characters.");
    if (scdthr < 0.0 || scdthr > 100.0)
        env->ThrowError("vsCnr2: scdthr must be between 0.0 and 100.0 (inclusive).");
    if (ln < 0 || ln > 255 ||
        lm < 0 || lm > 255 ||
        un < 0 || un > 255 ||
        um < 0 || um > 255 ||
        vn < 0 || vn > 255 ||
        vm < 0 || vm > 255)
        env->ThrowError("vsCnr2: ln, lm, un, um, vn, vm all must be between 0 and 255 (inclusive).");

    range_max = 1 << depth;
    const int pixelsize = sizeof(T);

    if (pixelsize == 2)
    {
        const int peak = range_max - 1;
        ln *= peak / 255;
        un *= peak / 255;
        vn *= peak / 255;
        lm *= peak / 255;
        um *= peak / 255;
        vm *= peak / 255;
    }

    curp_y = new T[static_cast<int64_t>((vi.width >> subsw)) * (vi.height >> subsh) * pixelsize];
    prevp_y = new T[static_cast<int64_t>((vi.width >> subsw)) * (vi.height >> subsh) * pixelsize];

    const int max_pixel_diff = (!scenechroma) ? 219 : (219 + (224 * 2)) >> (subsw + subsh);
    diff_max = static_cast<int64_t>((scdthr * vi.width * vi.height * max_pixel_diff) / 100.0) << (depth - 8);

    const int table = [&]()
    {
        if (pixelsize == 1)
            return 513;
        else
            switch (depth)
            {
                case 10: return 2049;
                case 12: return 8193;
                case 14: return 32769;
                default: return 131073;
            }
    }();

    table_y = new T[table];
    table_u = new T[table];
    table_v = new T[table];

    if (pixelsize == 1)
    {
        memset(table_y, 0, table);
        memset(table_u, 0, table);
        memset(table_v, 0, table);
    }
    else
    {
        memset16(table_y, 0, table);
        memset16(table_u, 0, table);
        memset16(table_v, 0, table);
    }

    for (int j = -ln; j <= ln; ++j)
    {
        if (mode[0] != 'x')
            table_y[j + range_max] = static_cast<int>(lm / 2 * (1 + cos(j * j * M_PI / (ln * ln))));
        else
            table_y[j + range_max] = static_cast<int>(lm / 2 * (1 + cos(j * M_PI / ln)));
    }

    for (int j = -un; j <= un; ++j)
    {
        if (mode[1] != 'x')
            table_u[j + range_max] = static_cast<int>(um / 2 * (1 + cos(j * j * M_PI / (un * un))));
        else
            table_u[j + range_max] = static_cast<int>(um / 2 * (1 + cos(j * M_PI / un)));
    }

    for (int j = -vn; j <= vn; ++j)
    {
        if (mode[2] != 'x')
            table_v[j + range_max] = static_cast<int>(vm / 2 * (1 + cos(j * j * M_PI / (vn * vn))));
        else
            table_v[j + range_max] = static_cast<int>(vm / 2 * (1 + cos(j * M_PI / vn)));
    }

    v8 = true;
    try { env->CheckVersion(8); }
    catch (const AvisynthError&) { v8 = false; };

    shift = 1LL << depth * 2;
    shift1 = 1 << (depth * 2 - 1);
    shift2 = depth * 2;
}

template <typename T>
vsCnr2<T>::~vsCnr2()
{
    delete[] curp_y;
    delete[] prevp_y;

    delete[] table_y;
    delete[] table_u;
    delete[] table_v;
}

template <typename T>
PVideoFrame __stdcall vsCnr2<T>::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame cur = child->GetFrame(n, env);

    if (n == 0)
    {
        prev = cur;
        return cur;
    }

    if (last_frame != n - 1)
    {
        prev = child->GetFrame(n - 1, env);
        downSampleLuma(prevp_y, prev);
    }

    PVideoFrame dst = (v8) ? env->NewVideoFrameP(vi, &cur) : env->NewVideoFrame(vi);
    env->BitBlt(dst->GetWritePtr(), dst->GetPitch(), cur->GetReadPtr(), cur->GetPitch(), cur->GetRowSize(), cur->GetHeight());

    downSampleLuma(curp_y, cur);

    const int stride_uv = cur->GetPitch(PLANAR_U) / sizeof(T);
    const int dst_stride_uv = dst->GetPitch(PLANAR_U) / sizeof(T);
    const int width_uv = cur->GetRowSize(PLANAR_U) / sizeof(T);
    const int height_uv = cur->GetHeight(PLANAR_U);

    const T* _curp_y = curp_y;
    const T* _prevp_y = prevp_y;

    const T* curp_u = reinterpret_cast<const T*>(cur->GetReadPtr(PLANAR_U));
    const T* prevp_u = reinterpret_cast<const T*>(prev->GetReadPtr(PLANAR_U));
    T* dstp_u = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_U));

    const T* curp_v = reinterpret_cast<const T*>(cur->GetReadPtr(PLANAR_V));
    const T* prevp_v = reinterpret_cast<const T*>(prev->GetReadPtr(PLANAR_V));
    T* dstp_v = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_V));

    int64_t diff_total = 0;

    for (int y = 0; y < height_uv; ++y)
    {
        for (int x = 0; x < width_uv; ++x)
        {
            const int diff_y = _curp_y[x] - _prevp_y[x];
            const int diff_u = curp_u[x] - prevp_u[x];
            const int diff_v = curp_v[x] - prevp_v[x];

            diff_total += abs(diff_y << (subsw + subsh));

            if (scenechroma)
                diff_total += static_cast<int64_t>(abs(diff_u)) + abs(diff_v);

            const int64_t weight_u = static_cast<int64_t>(table_y[diff_y + range_max]) * table_u[diff_u + range_max];
            const int64_t weight_v = static_cast<int64_t>(table_y[diff_y + range_max]) * table_v[diff_v + range_max];

            dstp_u[x] = (weight_u * prevp_u[x] + (shift - weight_u) * curp_u[x] + shift1) >> shift2;
            dstp_v[x] = (weight_v * prevp_v[x] + (shift - weight_v) * curp_v[x] + shift1) >> shift2;
        }

        if (diff_total > diff_max)
            return cur;

        curp_u += stride_uv;
        dstp_u += dst_stride_uv;
        prevp_u += stride_uv;

        curp_v += stride_uv;
        dstp_v += dst_stride_uv;
        prevp_v += stride_uv;

        _curp_y += width_uv;
        _prevp_y += width_uv;
    }

    last_frame = n;
    T* temp = curp_y;
    curp_y = prevp_y;
    prevp_y = temp;
    prev = dst;

    return dst;
}
    
AVSValue __cdecl Create_vsCnr2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    const VideoInfo& vi = clip->GetVideoInfo();

    if (vi.ComponentSize() == 1)
        return new vsCnr2<uint8_t>(
            clip,
            args[1].AsString("oxx"),
            args[2].AsFloat(10.0),
            args[3].AsInt(35),
            args[4].AsInt(192),
            args[5].AsInt(47),
            args[6].AsInt(255),
            args[7].AsInt(47),
            args[8].AsInt(255),
            args[9].AsBool(false),
            env);
    else
        return new vsCnr2<uint16_t>(
            clip,
            args[1].AsString("oxx"),
            args[2].AsFloat(10.0),
            args[3].AsInt(35),
            args[4].AsInt(192),
            args[5].AsInt(47),
            args[6].AsInt(255),
            args[7].AsInt(47),
            args[8].AsInt(255),
            args[9].AsBool(false),
            env);
}

const AVS_Linkage *AVS_linkage;

extern "C" __declspec(dllexport)
const char * __stdcall AvisynthPluginInit3(IScriptEnvironment *env, const AVS_Linkage *const vectors)
{
    AVS_linkage = vectors;
    
    env->AddFunction("vsCnr2", "c[mode]s[scdthr]f[ln]i[lm]i[un]i[um]i[vn]i[vm]i[sceneChroma]b", Create_vsCnr2, 0);
    return "vsCnr2";
}
