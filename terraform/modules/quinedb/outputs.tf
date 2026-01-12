# QuineDB Module Outputs

output "namespace" {
  description = "Kubernetes namespace"
  value       = kubernetes_namespace.quinedb.metadata[0].name
}

output "service_name" {
  description = "QuineDB service name"
  value       = kubernetes_service.quinedb.metadata[0].name
}

output "headless_service_name" {
  description = "QuineDB headless service name"
  value       = kubernetes_service.headless.metadata[0].name
}

output "statefulset_name" {
  description = "QuineDB StatefulSet name"
  value       = kubernetes_stateful_set.quinedb.metadata[0].name
}

output "connection_string" {
  description = "Internal connection string"
  value       = "${kubernetes_service.quinedb.metadata[0].name}.${kubernetes_namespace.quinedb.metadata[0].name}.svc.cluster.local:${var.quinedb_port}"
}
