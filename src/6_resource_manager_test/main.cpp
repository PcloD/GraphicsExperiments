#include <iostream>
#include <vector>
#include <string>

using ResourceID = uint32_t;

template <typename T>
class Handle;

template <typename T>
class ResourceCache
{
public:
	ResourceCache()
	{

	}

	~ResourceCache()
	{

	}

	virtual ResourceID load_internal(std::string name) = 0;

	Handle<T> load(std::string name)
	{
		ResourceID id = load_internal(name);
		return Handle<T>(id, this);
	}

	bool unload(ResourceID id)
	{
		if (m_resources.size() > 0 && id != UINT32_MAX)
		{
			ResourceEntry& entry = m_resources[id];

			decrement_ref(id);

			if (entry.ref_count == 0)
			{
				std::cout << "Ref Count Zero, Unloading..." << std::endl;
				delete entry.resource;
				m_resources.clear();
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}

	T* lookup(ResourceID id)
	{
		if (m_resources.size() > 0 && id != UINT32_MAX)
		{
			ResourceEntry& entry = m_resources[id];
			return entry.resource;
		}
		else
			return nullptr;
	}

	void increment_ref(ResourceID id)
	{
		if (m_resources.size() > 0 && id != UINT32_MAX)
		{
			m_resources[id].ref_count++;
		}
	}

	void decrement_ref(ResourceID id)
	{
		if (m_resources.size() > 0 && id != UINT32_MAX)
		{
			m_resources[id].ref_count--;
		}
	}

protected:
	struct ResourceEntry
	{
		T*		 resource;
		uint32_t ref_count;
	};

	std::vector<ResourceEntry> m_resources;
};

template <typename T>
class Handle
{
public:
	Handle() : m_id(UINT32_MAX), m_resource_cache(nullptr)
	{
	
	}

	Handle(ResourceID id, ResourceCache<T>* res_mgr) : m_id(id), m_resource_cache(res_mgr)
	{
		m_resource_cache->increment_ref(m_id);
	}

	Handle(const Handle<T>& other) : m_id(other.m_id), m_resource_cache(other.m_resource_cache)
	{
		//if (m_resource_manager)
		m_resource_cache->increment_ref(m_id);
	}

	~Handle()
	{
		unload();
	}

	operator bool() const
	{
		return ptr() != nullptr;
	}

	T* operator -> () const
	{
		//if (m_resource_manager)
			return m_resource_cache->lookup(m_id);
		//else
			//return nullptr;
	}

	Handle<T>& operator = (const Handle<T>& other)
	{
		m_resource_cache = other.m_resource_cache;
		m_id = other.m_id;

		//if (m_resource_manager)
			m_resource_cache->increment_ref(m_id);

		return *this;
	}

	T* ptr() const
	{
		//if (m_resource_manager)
			return m_resource_cache->lookup(m_id);
		//else
		//	return nullptr;
	}

	ResourceID id() const
	{
		return m_id;
	}

	void unload()
	{
		if (m_resource_cache)
		{
			if (m_resource_cache->unload(m_id))
			{
				m_id = UINT32_MAX;
			}
		}
	}

private:
	ResourceID		  m_id;
	ResourceCache<T>* m_resource_cache;
};

struct Texture
{
	int num;

	void foo()
	{
		std::cout << "foo called: " << num << std::endl;
	}
};

struct Material
{

};

struct Shader
{

};

class TextureCache: public ResourceCache<Texture>
{
public:
	virtual ResourceID load_internal(std::string name) override
	{
		ResourceEntry entry;

		entry.ref_count = 0;
		entry.resource = new Texture();
		entry.resource->num = rand();
		m_resources.push_back(entry);

		return m_resources.size() - 1;
	}
};

class MaterialCache : public ResourceCache<Material>
{
public:
	virtual ResourceID load_internal(std::string name) override
	{
		ResourceEntry entry;

		entry.ref_count = 0;
		entry.resource = new Material();
		m_resources.push_back(entry);

		return m_resources.size() - 1;
	}
};

class ShaderCache : public ResourceCache<Shader>
{
public:
	virtual ResourceID load_internal(std::string name) override
	{
		ResourceEntry entry;

		entry.ref_count = 0;
		entry.resource = new Shader();
		m_resources.push_back(entry);

		return m_resources.size() - 1;
	}
};

class ResourceManager
{
private:
	TextureCache  m_texture_cache;
	MaterialCache m_material_cache;
	ShaderCache   m_shader_cache;

public:
	ResourceManager()
	{

	}

	~ResourceManager()
	{

	}

	template <typename T>
	Handle<T> load(std::string name) {}

	template <>
	Handle<Texture> load<Texture>(std::string name)
	{
		return m_texture_cache.load(name);
	}

	template <>
	Handle<Material> load<Material>(std::string name)
	{
		return m_material_cache.load(name);
	}

	template <>
	Handle<Shader> load<Shader>(std::string name)
	{
		return m_shader_cache.load(name);
	}
};

struct Scene
{
	Handle<Texture> tex;

	Scene(ResourceManager& mgr)
	{
		tex = mgr.load<Texture>("asdas");
		//tex.unload();

		if (tex)
			tex->foo();
		else
			std::cout << "Invalid" << std::endl;
	}
};

int main()
{
	ResourceManager mgr;

	//{
	//	Handle<Texture> t1;

	//	{
	//		t1 = mgr.load("");

	//		if (t1)
	//			std::cout << "valid" << std::endl;

	//		{
	//			Handle<Texture> t2 = t1;
	//			t2->foo();
	//		}
	//	}

	//	if (t1)
	//		std::cout << "valid" << std::endl;
	//	else
	//		std::cout << "invalid" << std::endl;
	//}

	Scene* scene = new Scene(mgr);
	delete scene;

	std::cin.get();
	return 0;
}