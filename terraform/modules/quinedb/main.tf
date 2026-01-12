# QuineDB Application Module - Kubernetes resources for QuineDB

terraform {
  required_providers {
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.24"
    }
  }
}

# Namespace
resource "kubernetes_namespace" "quinedb" {
  metadata {
    name = var.namespace
    labels = {
      app     = "quinedb"
      managed = "terraform"
    }
  }
}

# ConfigMap for QuineDB configuration
resource "kubernetes_config_map" "quinedb" {
  metadata {
    name      = "quinedb-config"
    namespace = kubernetes_namespace.quinedb.metadata[0].name
  }

  data = {
    QUINE_PORT        = tostring(var.quinedb_port)
    QUINE_DATA_DIR    = "/app/data"
    QUINE_LOG_LEVEL   = var.log_level
  }
}

# Headless Service for StatefulSet
resource "kubernetes_service" "headless" {
  metadata {
    name      = "quinedb-headless"
    namespace = kubernetes_namespace.quinedb.metadata[0].name
    labels = {
      app = "quinedb"
    }
  }

  spec {
    selector = {
      app = "quinedb"
    }
    cluster_ip = "None"
    port {
      name        = "resp"
      port        = var.quinedb_port
      target_port = var.quinedb_port
    }
  }
}

# ClusterIP Service
resource "kubernetes_service" "quinedb" {
  metadata {
    name      = "quinedb"
    namespace = kubernetes_namespace.quinedb.metadata[0].name
    labels = {
      app = "quinedb"
    }
  }

  spec {
    selector = {
      app = "quinedb"
    }
    type = var.service_type
    port {
      name        = "resp"
      port        = var.quinedb_port
      target_port = var.quinedb_port
    }
  }
}

# StatefulSet
resource "kubernetes_stateful_set" "quinedb" {
  metadata {
    name      = "quinedb"
    namespace = kubernetes_namespace.quinedb.metadata[0].name
    labels = {
      app = "quinedb"
    }
  }

  spec {
    service_name = kubernetes_service.headless.metadata[0].name
    replicas     = var.replicas

    selector {
      match_labels = {
        app = "quinedb"
      }
    }

    template {
      metadata {
        labels = {
          app = "quinedb"
        }
      }

      spec {
        container {
          name              = "quinedb"
          image             = var.image
          image_pull_policy = var.image_pull_policy

          port {
            container_port = var.quinedb_port
            name           = "resp"
          }

          env_from {
            config_map_ref {
              name = kubernetes_config_map.quinedb.metadata[0].name
            }
          }

          volume_mount {
            name       = "data"
            mount_path = "/app/data"
          }

          resources {
            requests = {
              cpu    = var.cpu_request
              memory = var.memory_request
            }
            limits = {
              cpu    = var.cpu_limit
              memory = var.memory_limit
            }
          }

          liveness_probe {
            tcp_socket {
              port = var.quinedb_port
            }
            initial_delay_seconds = 15
            period_seconds        = 10
          }

          readiness_probe {
            tcp_socket {
              port = var.quinedb_port
            }
            initial_delay_seconds = 5
            period_seconds        = 5
          }
        }
      }
    }

    volume_claim_template {
      metadata {
        name = "data"
      }
      spec {
        access_modes       = ["ReadWriteOnce"]
        storage_class_name = var.storage_class
        resources {
          requests = {
            storage = var.storage_size
          }
        }
      }
    }
  }
}
