#version 330 core

layout(location = 0) out uvec2 outPrimitiveId;

uniform uvec2 uPrimitiveId;

void main()
{
    outPrimitiveId =
        uPrimitiveId;
}