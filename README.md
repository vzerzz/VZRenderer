# VZRenderer

纯C++实现的软光栅渲染器

项目笔记: https://vzerzz.github.io/Projects/VZRenderer/

主要实现的功能：

- Basic Render pipeline
- simple Orbital camera controls
- Homogeneous clipping and Back-Front culling
- Depth testing
- Perspective correct interpolation
- Tangent space normal mapping
- ACES tone mapping and gamma correction
- Blinn–Phong model
- Cubemap skybox
- Physically based rendering（PBR）
- Image-based lighting（IBL）
- Win32 api for window and UI

待实现的功能(可能会实现):
- MSAA等Anti-aliasing
- 双线性及三线性插值
- hdr
- ...


本项目主要用于学习以及实践所学知识 参照了tinyrenderer https://github.com/ssloy/tinyrenderer 以及 [@孙小磊](https://www.zhihu.com/people/sun-lei-22-19/posts) https://github.com/SunXLei/SRender 等大佬的作品 以及 https://learnopengl-cn.github.io/ 受益匪浅 十分感谢
