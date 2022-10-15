struct Disk
{
	float3 center;
	float3 normal;
	float halfx;
	float halfy;
};

Disk InitDisk(float3 center, float3 normal, float halfx, float halfy)
{
	Disk disk;

	disk.center = center;
	disk.normal = normal;
	disk.halfx = halfx;
	disk.halfy = halfy;

	return disk;
}

float IntersectSphere(Ray ray, float4 sph)
{
	float3 oc = ray.origin - sph.xyz;
	float b = dot(oc, ray.dir);
	float c = dot(oc, oc) - sph.w*sph.w;
	float h = b * b - c;
	if (h<0.0) return -1.0f;
	h = sqrt(h);
	return -b - h;
}

float IntersectPlane(Ray ray, float3 planeCenter, float3 planeNormal, out float3 hitPoint) {
	float denom = abs(dot(planeNormal, ray.dir));
	if (denom > 0.00001) {
		float3 diff= ray.origin - planeCenter;

		float t = dot(diff, planeNormal) / denom;
		if (t > 0.0f) {
			hitPoint = ray.origin + ray.dir * t;
			return 1.0f;
		}
	}
	return 0.0f;
}

float IntersectDisk(Ray ray, float3 diskCenter, float3 diskNormal, float diskRadius) {
	float3 hitPoint;
	float intensity = IntersectPlane(ray, diskCenter, diskNormal, hitPoint);
	if (intensity > 0.0f) {
		float dist = distance(hitPoint, diskCenter);
		if (dist < diskRadius) return 1.0f;
	}
	return 0.0f;
}

float SphereIlluminance(float intensity, Ray ray, float4 sphere)
{
	float result = 0.0f;
	result = IntersectSphere(ray, sphere);
	float dif = pow(distance(ray.origin, sphere.xyz), 2);
	float ratio = intensity / dif;
	return saturate(ratio * result);
}

void swap(inout float a, inout float b) {
	float tmp = a;
	a = b;
	b = tmp;
}

float IntersectBox(Ray r, float3 pos, float size) {

	float3 Min = pos - float3(size, size, size);
	float3 Max = pos + float3(size, size, size);

	float tMin = (Min.x - r.origin.x) / r.dir.x;
	float tMax = (Max.x - r.origin.x) / r.dir.x;

	if (tMin > tMax) swap(tMin, tMax);

	float tyMin = (Min.y - r.origin.y) / r.dir.y;
	float tyMax = (Max.y - r.origin.y) / r.dir.y;

	if (tyMin > tyMax) swap(tyMin, tyMax);

	if ((tMin > tyMax) || (tyMin > tMax))
		return 0.0f;

	if (tyMin > tMin)
		tMin = tyMin;

	if (tyMax < tMax)
		tMax = tyMax;

	float tzMin = (Min.z - r.origin.z) / r.dir.z;
	float tzMax = (Max.z - r.origin.z) / r.dir.z;

	if (tzMin > tzMax) swap(tzMin, tzMax);

	if ((tMin > tzMax) || (tzMin > tMax))
		return 0.0f;

	if (tzMin > tMin)
		tMin = tzMin;

	if (tzMax < tMax)
		tMax = tzMax;

	return 1.0f;
}