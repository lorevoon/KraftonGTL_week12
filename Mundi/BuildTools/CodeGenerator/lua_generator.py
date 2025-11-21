"""
Lua 바인딩 코드 생성 모듈
LUA_BIND_BEGIN / LUA_BIND_END 블록을 생성합니다.
"""

from jinja2 import Template
from header_parser import ClassInfo, Function


LUA_BIND_TEMPLATE = """
LUA_BIND_BEGIN({{ class_name }})
{
{%- for prop in properties %}
    {%- if prop.is_array_ptr %}
    AddPropertyArrayPtr<{{ class_name }}, {{ prop.inner_type }}>(
        T, "{{ prop.name }}", &{{ class_name }}::{{ prop.name }});
    {%- elif prop.is_pointer %}
    AddPropertyPtr<{{ class_name }}, {{ prop.base_type }}>(
        T, "{{ prop.name }}", &{{ class_name }}::{{ prop.name }});
    {%- elif prop.editable %}
    AddProperty<{{ class_name }}, {{ prop.type }}>(
        T, "{{ prop.name }}", &{{ class_name }}::{{ prop.name }});
    {%- else %}
    AddReadOnlyProperty<{{ class_name }}, {{ prop.type }}>(
        T, "{{ prop.name }}", &{{ class_name }}::{{ prop.name }});
    {%- endif %}
{%- endfor %}
{%- for func in functions %}
    {%- if func.return_type == 'void' %}
    AddAlias<{{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- else %}
    AddMethodR<{{ func.return_type }}, {{ class_name }}{{ func.get_parameter_types_string() }}>(
        T, "{{ func.display_name }}", &{{ class_name }}::{{ func.name }});
    {%- endif %}
{%- endfor %}
}
LUA_BIND_END()
"""


class LuaBindingGenerator:
    """Lua 바인딩 코드 생성기"""

    def __init__(self):
        self.template = Template(LUA_BIND_TEMPLATE)

    def generate(self, class_info: ClassInfo) -> str:
        """ClassInfo로부터 LUA_BIND_BEGIN 블록 생성"""
        # LuaBind 메타데이터가 있는 함수만 필터링
        lua_functions = [
            f for f in class_info.functions
            if f.metadata.get('lua_bind', False)
        ]

        # 모든 UPROPERTY를 Lua에 바인딩 (프로퍼티 정보 준비)
        lua_properties = []
        for prop in class_info.properties:
            # TArray<T*> 타입 감지
            is_array_ptr = False
            inner_type = None
            if 'TArray<' in prop.type and '*>' in prop.type:
                # TArray<USound*> -> inner_type = "USound"
                import re
                match = re.search(r'TArray<\s*(\w+)\s*\*\s*>', prop.type)
                if match:
                    is_array_ptr = True
                    inner_type = match.group(1)

            prop_info = {
                'name': prop.name,
                'type': prop.type,
                'editable': prop.editable,
                'is_array_ptr': is_array_ptr,
                'inner_type': inner_type,
                'is_pointer': '*' in prop.type and not is_array_ptr,
                'base_type': prop.type.replace('*', '').strip() if '*' in prop.type else prop.type
            }
            lua_properties.append(prop_info)

        if not lua_functions and not lua_properties:
            # 바인딩할 것이 아무것도 없으면 빈 블록
            return f"""
LUA_BIND_BEGIN({class_info.name})
{{
    // No properties or functions to bind
}}
LUA_BIND_END()
"""

        return self.template.render(
            class_name=class_info.name,
            properties=lua_properties,
            functions=lua_functions
        )
