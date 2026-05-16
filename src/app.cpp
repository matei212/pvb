#include "app.hpp"

void App::Start()
{
    m_UI.Init();

    while (!glfwWindowShouldClose(m_UI.GetWindow())) {
        glfwPollEvents();

        m_UI.Update();

        int width, height;
        glfwGetFramebufferSize(m_UI.GetWindow(), &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_UI.Draw();

        glfwSwapBuffers(m_UI.GetWindow());
    }

    m_UI.Destroy();
}
