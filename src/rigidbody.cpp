#include "rigidbody.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

RigidBody::RigidBody(Entity *entity)
: Model(entity)
{
	sleep = false;
	gravity = true;
	mass = 10.0f;
	restitution = 1.0f; // boxes should never rest
	kfriction = 0.5f;
	float height = 10.0f / UNITS_TO_METERS;
	float width = 10.0f / UNITS_TO_METERS;
	float depth = 10.0f / UNITS_TO_METERS;

	world_tensor.m[0] = 0.0f;
	world_tensor.m[1] = 0.0f;
	world_tensor.m[2] = 0.0f;
	world_tensor.m[3] = 0.0f;
	world_tensor.m[4] = 0.0f;
	world_tensor.m[5] = 0.0f;
	world_tensor.m[6] = 0.0f;
	world_tensor.m[7] = 0.0f;
	world_tensor.m[8] = 0.0f;

	inverse_tensor.m[0] = 12.0f / (mass *  (height * height + depth * depth));
	inverse_tensor.m[1] = 0;
	inverse_tensor.m[2] = 0;

	inverse_tensor.m[3] = 0;
	inverse_tensor.m[4] = 12.0f / (mass *  (width * width + depth * depth));
	inverse_tensor.m[5] = 0;
	
	inverse_tensor.m[6] = 0;
	inverse_tensor.m[7] = 0;
	inverse_tensor.m[8] = 12.0f / (mass *  (width * width + height * height));
}

void RigidBody::recalc()
{
	float height = (aabb[7].y - aabb[0].y);
	float width = (aabb[7].x - aabb[0].x);
	float depth = (aabb[7].z - aabb[0].z);
	mass = (height * width * depth) / 800.0f;
	if (mass == 0.0f)
		mass = 10.0f;

	inverse_tensor.m[0] = 1.0f / (mass *  (height * height + depth * depth));
	inverse_tensor.m[1] = 0;
	inverse_tensor.m[2] = 0;

	inverse_tensor.m[3] = 0;
	inverse_tensor.m[4] = 1.0f / (mass *  (width * width + depth * depth));
	inverse_tensor.m[5] = 0;
	
	inverse_tensor.m[6] = 0;
	inverse_tensor.m[7] = 0;
	inverse_tensor.m[8] = 1.0f / (mass *  (width * width + height * height));
}

/*
	Integrate physical quantaties over time by a fixed time step

	Writes to acceleration, angular_acceleration, angular velocity, velocity, position,
	and morientation
*/
void RigidBody::integrate(float time)
{
	matrix3 rotation;
	vec3 acceleration, angular_acceleration;

	if (sleep)
		return;

	//translational
	acceleration = net_force / mass;
	velocity = velocity + acceleration * time;
	old_position = entity->position;
	entity->position = entity->position + velocity * time * UNITS_TO_METERS;

	//rotational
	angular_acceleration = world_tensor * net_torque;
	angular_velocity = angular_velocity + angular_acceleration * time;
	rotation.star(angular_velocity);

	old_orientation = morientation;
	morientation = morientation + morientation * rotation * time;
	morientation.normalize();
	world_tensor = morientation * inverse_tensor * morientation.transpose();
}

/*
	Detects a collision with a plane and applies physical impulse response
*/
bool RigidBody::collision_detect(Plane &p)
{
	for( int i = 0;	i < 8; i++)
	{
		// make center origin
		vec3 point = center + aabb[i];

		//rotate around origin
		point = morientation * point;

		// rotate center around about true origin
		vec3 offset = morientation * center;

		// translate back to local coordinate origin
		point = point - offset;

		// translate to world coordinates
		point = point + entity->position;

		float d = point * p.normal + p.d;

		if ( d < -0.25f )
		{
			// Simulated too far
			return true;
		}
		else if ( d < 0.0f )
		{
			// Colliding
			// To handle multiple collision we need to store temp velocities

			// convert rotated point back to local coordinates
			point = point - entity->position;

			// convert point back to radius from center
			point = point + offset;

			// convert to meters
			point = point * (1.0f / UNITS_TO_METERS);

			// apply impulse to plane and radius vector
			impulse(p, point);
			entity->position = old_position;
			morientation = old_orientation;
		}
	}
	return false;
}

/*
	Applys collision impulse to a vertex
	radius must be in units of meters from CM
*/
void RigidBody::impulse(Plane &plane, vec3 &radius)
{
	float	impulse_numerator;
	float	impulse_denominator;
	vec3	impulse_momentum;

	vec3	vertex_velocity = velocity + vec3::crossproduct(angular_velocity, radius);

	// coefficient of resistution * -relative velocity
	impulse_numerator = -(1.0f + restitution) * (vertex_velocity * plane.normal);

	// 1/mass + N dot [((1/I)(radius cross normal)) cross radius] -- units of momentum p = mv
	impulse_denominator = (1.0f / mass) + plane.normal *
		vec3::crossproduct(world_tensor * vec3::crossproduct(radius, plane.normal), radius);
    
    impulse_momentum = plane.normal * (impulse_numerator/impulse_denominator);

	// apply impulse to primary quantities
    velocity += impulse_momentum / mass;
    angular_velocity = world_tensor * vec3::crossproduct(radius, -impulse_momentum);
}

void RigidBody::impulse(RigidBody &rigid, vec3 &point)
{
	float	impulse_numerator;
	float	impulse_denominator;
	vec3	impulse_force;
	vec3	local_point = (point - entity->position) * (1.0f / UNITS_TO_METERS);

	vec3	relative_velocity = (velocity - rigid.velocity) + vec3::crossproduct(angular_velocity, local_point) - vec3::crossproduct(rigid.angular_velocity, local_point);
	vec3	normal = rigid.entity->position - point;
	normal.normalize();

	// coefficient of resistution * -relative velocity
	impulse_numerator = -(1.0f + restitution) * (relative_velocity * normal);

	// 1/mass + N dot [((1/I)(radius cross normal)) cross radius] -- units of momentum p = mv
	impulse_denominator = (1.0f / mass) + (1.0f / rigid.mass)
		+ normal * vec3::crossproduct(world_tensor * vec3::crossproduct(local_point, normal), local_point)
		+ normal * vec3::crossproduct(rigid.world_tensor * vec3::crossproduct(local_point, normal), local_point);
    
    impulse_force = normal * (impulse_numerator/impulse_denominator);

	// apply impulse to primary quantities
    velocity += impulse_force * (1.0f / mass);
    angular_velocity = world_tensor * vec3::crossproduct(local_point, -impulse_force);
	rigid.velocity -= impulse_force * (1.0f / rigid.mass);
	rigid.angular_velocity = rigid.world_tensor * vec3::crossproduct(local_point, impulse_force);
	sleep = false;
	rigid.sleep = false;
	gravity = true;
}

void RigidBody::impulse(RigidBody &rigid, vec3 &point, Plane &plane)
{
	float	impulse_numerator;
	float	impulse_denominator;
	vec3	impulse_force;
	vec3	local_point = (point - entity->position);

	vec3	relative_velocity = (velocity - rigid.velocity) + vec3::crossproduct(angular_velocity, local_point) - vec3::crossproduct(rigid.angular_velocity, local_point);
	vec3	normal = plane.normal;
	normal.normalize();

	// coefficient of resistution * -relative velocity
	impulse_numerator = -(1.0f + restitution) * (relative_velocity * normal);

	// 1/mass + N dot [((1/I)(radius cross normal)) cross radius] -- units of momentum p = mv
	impulse_denominator = (1.0f / mass) + (1.0f / rigid.mass)
		+ normal * vec3::crossproduct(world_tensor * vec3::crossproduct(local_point, normal), local_point)
		+ normal * vec3::crossproduct(rigid.world_tensor * vec3::crossproduct(local_point, normal), local_point);
    
	impulse_force = normal * (impulse_numerator/impulse_denominator);

	// apply impulse to primary quantities
	velocity += impulse_force * (1.0f / mass);
	angular_velocity = world_tensor * vec3::crossproduct(local_point, -impulse_force);
	rigid.velocity -= impulse_force * (1.0f / rigid.mass);
	rigid.angular_velocity = rigid.world_tensor * vec3::crossproduct(local_point, impulse_force);
	sleep = false;
	rigid.sleep = false;
	gravity = true;
}


/*
	Detects a collision between current entity and a point
*/
bool RigidBody::collision_detect(vec3 &v)
{
	vec3 a, b;

	a = aabb[0] + entity->position + center;
	b = aabb[7] + entity->position + center;

	if ( (v.x > a.x) && (v.x < b.x) )
	{
		if ( (v.y > a.y) && (v.y < b.y) )
		{
			if ( (v.z > a.z) && (v.z < b.z) )
			{
				return true;
			}
		}
	}
	return false;
}

/*
	Detects a collision between current entity and another
*/
bool RigidBody::collision_detect_simple(RigidBody &body)
{
	for( int i = 0;	i < 8; i++)
	{
		vec3 point = body.aabb[i] + body.center;
		point = (body.morientation * point) + body.entity->position + body.center;
		if ( collision_detect(point) )
		{
			impulse(body, point);
			entity->position = old_position;
			morientation = old_orientation;
			return true;
		}
	}
	return false;
}

bool RigidBody::collision_detect(RigidBody &body)
{
	Plane plane[6];
	vec3 point;

	// Bounding box planes
	plane[0] = vec4(0.0f, 1.0f, 0.0f, -0.5f); // up
	plane[1] = vec4(0.0f, -1.0f, 0.0f, 0.5f); // down
	plane[2] = vec4(1.0f, 0.0f, 0.0f, -0.5f); // right
	plane[3] = vec4(-1.0f, 0.0f, 0.0f, 0.5f); // left
	plane[4] = vec4(0.0f, 0.0f, 1.0f, -0.5f); // far
	plane[5] = vec4(0.0f, 0.0f, -1.0f, 0.5f); // near


	// Rotate and translate planes to world space
	for(int i = 0; i < 6; i++)
	{
		point = plane[i].normal * 0.5f;						// point on plane
		point = morientation * point + entity->position;				// rotate point
		plane[i].normal = morientation * plane[i].normal;	// rotate normal
		plane[i].normal.normalize();
		plane[i].d = -(plane[i].normal * point);			// recalculate D
	}

	// Check if point of body is inside our bounding box
	for(int i = 0; i < 8; i++)
	{
		const float padding = 0.0125f;
		float depth = 1000.0f;
		int count = 0;
		int closest = 0;

		// oriented body point in world space
		point = body.morientation * body.aabb[i] + body.entity->position;

		for(int j = 0; j < 6; j++)
		{
			float distance = plane[j].normal * point + plane[j].d;
			if ( distance < padding)
			{
				if (depth > abs32(distance))
				{
					depth = abs32(distance);
					closest = j;
				}
				count++;
			}
		}

		// We had a point inside all 6 planes
		if (count == 6)
		{
			// Too deep, divide time step
			if (depth > 0.125f)
				return true;

			printf("body body impact\n");
			impulse(body, point, plane[closest]);
			entity->position = old_position;
			morientation = old_orientation;
		}
	}
	return false;
}

void RigidBody::frame2ent(Frame *camera, button_t &keyboard)
{
	vec3		forward = camera->forward;
	vec3		up(0.0f, 1.0f, 0.0f);
	vec3		right;

//	forward.y = 0.0f;
//	forward.normalize();
	right = vec3::crossproduct(forward, up);
	right.normalize();
	if (keyboard.control == false)
	{
		entity->rigid->sleep = false;
		entity->rigid->gravity = true;
		camera->pos = entity->position;

		morientation.m[0] = right.x;
		morientation.m[1] = right.y;
		morientation.m[2] = right.z;

		morientation.m[3] = camera->up.x;
		morientation.m[4] = camera->up.y;
		morientation.m[5] = camera->up.z;

		morientation.m[6] = -forward.x;
		morientation.m[7] = -forward.y;
		morientation.m[8] = -forward.z;
	}
	else
	{
//		entity->position = camera->pos;
	}
}

void RigidBody::save_config(cfg_t &config)
{
	config.morientation = morientation;
	config.world_tensor = world_tensor;
	config.velocity = velocity;
	config.angular_velocity = angular_velocity;
	config.position = entity->position;
}

void RigidBody::load_config(cfg_t &config)
{
	morientation = config.morientation;
	world_tensor = config.world_tensor;
	velocity = config.velocity;
	angular_velocity = config.angular_velocity;
	entity->position = config.position;
}


void RigidBody::seek(vec3 &position)
{
	vec3 direction = position - entity->position;
	direction.normalize();
	direction *= 8.0f; // speed
	net_force += direction - velocity;
}

void RigidBody::flee(vec3 &position)
{
	vec3 direction = entity->position - position;
	direction.normalize();
	direction *= 8.0f; // speed
	net_force += direction - velocity;
}

//not quite right yet
void RigidBody::arrive(vec3 &position)
{
	vec3 direction = position - entity->position;
	float distance = direction.magnitude();
	float speed = distance / 5.0f;
	if (speed > 8.0f)
		speed = 8.0f;
	direction = (direction / distance) * speed;
	net_force += direction - velocity;


	// visual doesnt affect above at all
	direction.y = 0.0f;
	vec3 up(0.0f, 1.0f, 0.0f);
	vec3 right = vec3::crossproduct(up, direction);
	right.normalize();
	morientation.m[0] = right.x;
	morientation.m[1] = right.y;
	morientation.m[2] = right.z;
	morientation.m[7] = direction.x;
	morientation.m[8] = 0.0f;
	morientation.m[9] = direction.z;
}

void RigidBody::pursue(Entity &target)
{
	vec3 direction = target.position - entity->position;
	float predict_time = direction.magnitude() / ( 8.0f + 8.0f );
	vec3 position = target.position + target.rigid->velocity * predict_time;
	seek(position);
}

void RigidBody::evade(Entity &target)
{
	vec3 direction = target.position - entity->position;
	float predict_time = direction.magnitude() / ( 8.0f + 8.0f );
	vec3 position = target.position + target.rigid->velocity * predict_time;

	flee(position);
}

void RigidBody::move(Frame &camera, button_t &keyboard)
{
	vec3	forward = camera.forward;
	vec3	right = vec3::crossproduct(camera.up, camera.forward);
	bool	moved = false;
	bool	jumped = false;

	sleep = false;
#define SPEED (1.0f)
	if (keyboard.up)
	{
		velocity += -forward * SPEED;
		moved = true;
	}
	if (keyboard.down)
	{
		velocity += forward * SPEED;
		moved = true;
	}
	if (keyboard.left)
	{
		velocity += -right * SPEED;
		moved = true;
	}
	if (keyboard.right)
	{
		velocity += right * SPEED;
		moved = true;
	}
	if (keyboard.enter)
	{
		jumped = true;
		moved = true;
	}
	if (keyboard.shift)
	{
		velocity.y += -SPEED;
		moved = true;
	}

	float speed = velocity.magnitude();

	if ( speed > 6.0f)
	{
		velocity.x *= (6.0f / speed);
		velocity.z *= (6.0f / speed);
	}

	if (moved)
	{
		if (jumped)
			velocity.y = 2.5f;
		else
			velocity.y = 0.1f;
	}

	if (moved == false)
	{
		velocity.x *= 0.5f;
		velocity.z *= 0.5f;
	}
}

void RigidBody::move(button_t &keyboard)
{
	vec3	right(morientation.m[0], morientation.m[1], morientation.m[2]);
	vec3	up(morientation.m[3], morientation.m[4], morientation.m[5]);
	vec3	forward(morientation.m[6], morientation.m[7], morientation.m[8]);

	sleep = false;
#define SPEED (1.0f) // meters per second
	if (keyboard.up)
	{
		entity->position += -forward * SPEED;
	}
	if (keyboard.down)
	{
		entity->position += forward * SPEED;
	}
	if (keyboard.left)
	{
		entity->position += -right * SPEED;
	}
	if (keyboard.right)
	{
		entity->position += right * SPEED;
	}
	if (keyboard.enter)
		entity->position += up * SPEED;
	if (keyboard.shift)
		entity->position += -up * SPEED;
}