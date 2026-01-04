# Deploying QuineDB on Kubernetes

## Prerequisites
*   Docker installed
*   Minikube or Kind (for local testing) or a cloud K8s cluster
*   `kubectl` installed

## Steps

### 1. Build Docker Image
Since we are using a local image, we need to point the Docker build to the Minikube daemon or load the image.

**Option A: Minikube**
```bash
eval $(minikube docker-env)
docker build -t quine-db:latest .
```

**Option B: Kind**
```bash
docker build -t quine-db:latest .
kind load docker-image quine-db:latest
```

### 2. Deploy Manifests
```bash
kubectl apply -f k8s/statefulset.yaml
kubectl apply -f k8s/service.yaml
```

### 3. Verify Deployment
```bash
kubectl get pods -w
# Wait for quine-db-0 to be Running
```

### 4. Connect
You can verify the connection using the `client.yaml` pod or port-forwarding.

**Method A: Port Forwarding**
```bash
kubectl port-forward svc/quine-db-service 6379:6379
# In another terminal:
redis-cli -p 6379
```

**Method B: Internal Client**
```bash
kubectl apply -f k8s/client.yaml
kubectl exec -it deployment/quine-client-demo -- redis-cli -h quine-db-service
```
