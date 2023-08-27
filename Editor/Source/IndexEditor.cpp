#include <IndexEngine.h>
#include <Index/Core/EntryPoint.h>
#include "Editor.h"

Index::Application* Index::CreateApplication()
{
    return new Editor();
}
