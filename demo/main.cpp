#include "hurdy_gurdy.h"

constexpr double rt3 = 1.73205080757;

static hg::ModelData generate_sphere(const f32 radius, u32 fidelity) {
    debug_assert(radius > 0);
    debug_assert(fidelity >= 3);

    std::vector<hg::ModelVertex> vertices = {};
    vertices.reserve(2 + fidelity * fidelity);

    glm::vec3 point = {0.0f, -radius, 0.0f};
    vertices.emplace_back(point, point, glm::vec2{});
    for (u32 i = 0; i < fidelity; ++i) {
        f32 h = -std::cos(glm::pi<f32>() * i / fidelity);
        f32 r = std::sin(glm::pi<f32>() * i / fidelity);
        for (u32 j = 0; j < fidelity; ++j) {
            point = glm::vec3{r * std::cos(glm::tau<f32>() * j / fidelity), h, r * std::sin(glm::tau<f32>() * j / fidelity)};
            vertices.emplace_back(point * radius, point, glm::vec2{});
        }
    }
    point = {0.0f, radius, 0.0f};
    vertices.emplace_back(point, point, glm::vec2{});

    std::vector<u32> indices = {};
    indices.reserve(fidelity * (fidelity + 2) * 3);
    for (u32 i = 0; i < fidelity; ++i) {
        indices.push_back(0);
        indices.push_back((i + 1) % fidelity + 1);
        indices.push_back((i + 2) % fidelity + 1);
    }
    for (u32 i = 1; i <= fidelity * (fidelity - 1); ++i) {
        indices.push_back(i);
        indices.push_back(i + fidelity);
        indices.push_back(i + 1);

        indices.push_back(i + 1);
        indices.push_back(i + fidelity);
        indices.push_back(i + fidelity + 1);
    }
    u32 top_index = 1 + fidelity * fidelity;
    for (u32 i = 0; i < fidelity; ++i) {
        indices.push_back(top_index);
        indices.push_back(top_index - ((i + 1) % fidelity + 1));
        indices.push_back(top_index - ((i + 2) % fidelity + 1));
    }

    return {std::move(indices), std::move(vertices)};
}

int main() {
    const auto engine = hg::Engine::create();
    defer(engine.destroy());

    auto window = hg::Window::create(engine, 1920, 1080);
    defer(window.destroy(engine));

    auto pbr_pipeline = hg::PbrPipeline::create(engine, window);
    defer(pbr_pipeline.destroy(engine));

    std::array<u32, 4> sphere_color = {};
    sphere_color.fill(0xff44ccff);
    pbr_pipeline.load_texture_from_data(engine, sphere_color.data(), {2, 2, 1}, vk::Format::eR8G8B8A8Srgb, 4);
    constexpr usize sphere_texture = 0;
    pbr_pipeline.load_texture(engine, "../assets/hexagon_models/Textures/hexagons_medieval.png");
    constexpr usize hex_tex = 1;

    const auto sphere_model = generate_sphere(0.5f, 32);
    pbr_pipeline.load_model_from_data(engine, sphere_model.indices, sphere_model.vertices, sphere_texture, 0.04f, 1.0f);
    constexpr usize sphere = 0;

    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", hex_tex);
    constexpr usize grass = 1;
    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/decoration/nature/tree_single_A.gltf", hex_tex);
    constexpr usize tree = 2;

    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", hex_tex);
    constexpr usize building = 3;
    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", hex_tex);
    constexpr usize tower = 4;
    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_blacksmith_blue.gltf", hex_tex);
    constexpr usize blacksmith = 5;
    pbr_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_castle_blue.gltf", hex_tex);
    constexpr usize castle = 6;

    pbr_pipeline.update_projection(engine, glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window.extent.width) / static_cast<f32>(window.extent.height), 0.1f, 100.f));

    hg::Cameraf camera = {};
    camera.translate({0.0f, -2.0f, -4.0f});

    hg::Transform3Df sphere_transform = {
        .position = {0.0f, -2.0f, 0.0f},
    };

    glm::dvec2 cursor_pos = {};
    glfwGetCursorPos(window.window, &cursor_pos.x, &cursor_pos.y);

    hg::Clock clock = {};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(window.window)) {
        clock.update();
        const f64 delta = clock.delta_sec();
        const f32 delta32 = static_cast<f32>(delta);
        if (time_count >= 1.0) {
            std::cout << std::format("avg: {}ms\n", 1000.0 / frame_count);
            frame_count = 0;
            time_count -= 1.0;
        }
        time_count += delta;
        ++frame_count;

        glfwPollEvents();

        if (glfwGetKey(window.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            sphere_transform.translate({-delta32, 0.0f, 0.0f});
        }
        if (glfwGetKey(window.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            sphere_transform.translate({delta32, 0.0f, 0.0f});
        }
        if (glfwGetKey(window.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            sphere_transform.translate({0.0f, 0.0f, -delta32});
        }
        if (glfwGetKey(window.window, GLFW_KEY_UP) == GLFW_PRESS) {
            sphere_transform.translate({0.0f, 0.0f, delta32});
        }

        constexpr f32 speed = 2.0f;
        if (glfwGetKey(window.window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);
        }
        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos = {};
        glfwGetCursorPos(window.window, &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif = new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        }
        if (cursor_dif.y != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));
        }

        const bool present_success = window.submit_frame(engine, [&](const vk::CommandBuffer cmd) {
            pbr_pipeline.queue_light({-2.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});
            pbr_pipeline.queue_light({2.5f, -2.0f, 2.25f}, {glm::vec3{1.0f, 0.2f, 0.0f} * 10.0f});

            pbr_pipeline.queue_model(sphere, sphere_transform);

            pbr_pipeline.queue_model(grass, {.position = {0.0f, 0.0f, 0.0f}});

            pbr_pipeline.queue_model(grass, {.position = {-1.0f, -0.25f, rt3}});
            pbr_pipeline.queue_model(blacksmith, {.position = {-1.0f, -0.25f, rt3}});

            pbr_pipeline.queue_model(grass, {.position = {1.0f, -0.5f, rt3}});
            pbr_pipeline.queue_model(castle, {.position = {1.0f, -0.5f, rt3}});

            pbr_pipeline.queue_model(grass, {.position = {-2.0f, -0.1f, 0.0f}});
            pbr_pipeline.queue_model(building, {.position = {-2.0f, -0.1f, 0.0f}});
            pbr_pipeline.queue_model(tree, {.position = {-2.0f - 0.75f, -0.1f, 0.0f - 0.25f}});

            pbr_pipeline.queue_model(grass, {.position = {2.0f, -0.25f, 0.0f}});
            pbr_pipeline.queue_model(tower, {.position = {2.0f, -0.25f, 0.0f}});
            pbr_pipeline.queue_model(tree, {.position = {2.0f - 0.75f, -0.25f, 0.0f + 0.25f}});
            pbr_pipeline.queue_model(tree, {.position = {2.0f + 0.75f, -0.25f, 0.0f - 0.25f}});

            pbr_pipeline.render(cmd, engine, window, camera);
        });

        if (!present_success) {
            debug_assert(engine.queue != nullptr);
            const auto wait_result = engine.queue.waitIdle();
            critical_assert(wait_result == vk::Result::eSuccess);

            for (int width = 0, height = 0; width == 0 || height == 0;) {
                debug_assert(window.window != nullptr);
                glfwGetWindowSize(window.window, &width, &height);
                glfwPollEvents();
            };

            window.resize(engine);
            pbr_pipeline.resize(engine, window);
            pbr_pipeline.update_projection(engine,
                                           glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window.extent.width) / static_cast<f32>(window.extent.height), 0.1f, 100.f));
        }
    }

    const auto wait_result = engine.device.waitIdle();
    critical_assert(wait_result == vk::Result::eSuccess);
}
